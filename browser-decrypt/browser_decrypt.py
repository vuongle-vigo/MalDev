#!/usr/bin/env python3
"""
browser_decrypt.py - Decrypt v10/v20 encrypted fields in browser sqlite dumps.

Input layout (as produced by BrowserExtract):
    <input>/Chrome/key_v10_hex.txt        32-byte AES-256 key, hex (already DPAPI/app-bound decrypted)
    <input>/Chrome/key_v20_hex.txt        32-byte app-bound AES-256 key, hex
    <input>/Chrome/Local State
    <input>/Chrome/Default/Cookies        sqlite with v10/v20 encrypted blobs
    <input>/Chrome/Default/Login Data
    <input>/Chrome/Default/Web Data
    <input>/Edge/Default/... , <input>/Edge/Profile 1/... etc.

Blob format (both v10 and v20): b"v10"/b"v20" || 12-byte nonce || ciphertext || 16-byte GCM tag,
AES-256-GCM, no AAD. v10 -> key_v10, v20 -> key_v20 (app-bound).

Newer Chromium cookies bind each value to its domain: plaintext = SHA-256(host_key) || value.
Verified against a live dump: 797/797 cookies follow that layout.

Output:
    <output>/<Browser>/<Profile>/<db>     copy of the sqlite with encrypted fields replaced by plaintext
    <output>/exports/<Browser>_<Profile>_*.csv

Usage:
    python browser_decrypt.py <input_folder> [-o output_folder]
"""

import argparse
import csv
import hashlib
import json
import os
import shutil
import sqlite3
import sys
import tempfile
from datetime import datetime, timedelta, timezone
from pathlib import Path

try:
    from cryptography.hazmat.primitives.ciphers.aead import AESGCM
except ImportError:
    print("[!] missing dependency: pip install cryptography", file=sys.stderr)
    sys.exit(1)

PREFIXES = (b"v10", b"v20")
PREFIX_LEN = 3
NONCE_LEN = 12
TAG_LEN = 16
HOST_BINDING_LEN = 32  # SHA-256(host_key) prepended to cookie plaintexts (Chromium 138+)

TARGET_DBS = ("Cookies", "Login Data", "Web Data", "History", "Bookmarks")


def chrome_time_to_iso(us_since_1601: int) -> str:
    """Chrome timestamps are microseconds since 1601-01-01 UTC."""
    if not us_since_1601:
        return ""
    try:
        epoch = datetime(1601, 1, 1, tzinfo=timezone.utc) + timedelta(microseconds=us_since_1601)
        return epoch.strftime("%Y-%m-%d %H:%M:%S")
    except (OverflowError, ValueError):
        return str(us_since_1601)


def load_keys(browser_dir: Path) -> dict:
    keys = {}
    for ver in ("v10", "v20"):
        key_file = browser_dir / f"key_{ver}_hex.txt"
        if key_file.is_file():
            try:
                key = bytes.fromhex(key_file.read_text().strip())
                if len(key) == 32:
                    keys[ver] = key
                else:
                    print(f"[!] {key_file}: expected 32 bytes, got {len(key)}")
            except ValueError:
                print(f"[!] {key_file}: invalid hex")
    return keys


def decrypt_blob(blob: bytes, keys: dict) -> bytes | None:
    """Return decrypted plaintext, or None if not a v10/v20 blob / no key / bad tag."""
    if not isinstance(blob, (bytes, bytearray)) or len(blob) <= PREFIX_LEN + NONCE_LEN + TAG_LEN:
        return None
    prefix = bytes(blob[:PREFIX_LEN])
    if prefix not in PREFIXES or prefix.decode() not in keys:
        return None
    try:
        return AESGCM(keys[prefix.decode()]).decrypt(
            bytes(blob[PREFIX_LEN:PREFIX_LEN + NONCE_LEN]),
            bytes(blob[PREFIX_LEN + NONCE_LEN:]),
            None,
        )
    except Exception:
        return None  # GCM tag mismatch: wrong key or corrupted blob


def strip_host_binding(plaintext: bytes, host_key: str) -> bytes:
    """Newer Chromium prepends SHA-256(host_key) to cookie values; strip when present."""
    if len(plaintext) > HOST_BINDING_LEN and host_key:
        binding = hashlib.sha256(host_key.encode("utf-8")).digest()
        if plaintext[:HOST_BINDING_LEN] == binding:
            return plaintext[HOST_BINDING_LEN:]
    return plaintext


def table_columns(con: sqlite3.Connection, table: str) -> list[str]:
    return [r[1] for r in con.execute(f'PRAGMA table_info("{table}")')]


def q(ident: str) -> str:
    return '"' + ident.replace('"', '""') + '"'


def decrypt_database(src: Path, dst: Path, keys: dict, stats: dict) -> None:
    """Copy sqlite to dst, decrypt every v10/v20 blob found in any table/column."""
    shutil.copy2(src, dst)
    for suffix in ("-wal", "-shm"):
        if Path(str(src) + suffix).is_file():
            shutil.copy2(str(src) + suffix, str(dst) + suffix)
    con = sqlite3.connect(str(dst))
    try:
        tables = [r[0] for r in con.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' "
            "AND sql NOT LIKE '%WITHOUT ROWID%'")]
        for table in tables:
            cols = table_columns(con, table)
            if not cols:
                continue
            # host column feeding the cookie domain-binding, if present
            host_col = "host_key" if "host_key" in cols else None
            host_sel = ", " + q(host_col) if host_col else ""
            for col in cols:
                rows = con.execute(
                    "SELECT rowid, " + q(col) + host_sel + " FROM " + q(table) +
                    " WHERE " + q(col) + " IS NOT NULL"
                ).fetchall()
                updates = []
                for row in rows:
                    if host_col:
                        rowid, blob, host = row[0], row[1], (row[2] or "")
                    else:
                        rowid, blob, host = row[0], row[1], ""
                    pt = decrypt_blob(blob, keys)
                    if pt is None:
                        continue
                    if table == "cookies":
                        pt = strip_host_binding(pt, host)
                    updates.append((pt, rowid))
                if updates:
                    con.executemany(
                        "UPDATE " + q(table) + " SET " + q(col) + "=? WHERE rowid=?", updates)
                    n = len(updates)
                    stats.setdefault(f"{table}.{col}", 0)
                    stats[f"{table}.{col}"] += n
        con.commit()
    finally:
        con.close()


def export_csv(path: Path, header: list[str], rows: list) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def export_credentials(src: Path, keys: dict, browser: str, profile: str, out_dir: Path) -> dict:
    """Human-readable CSV exports for the known tables (read from a temp copy)."""
    counts = {}
    tmp = Path(tempfile.mkdtemp()) / src.name
    shutil.copy2(src, tmp)
    con = sqlite3.connect(str(tmp))
    con.text_factory = lambda b: b.decode("utf-8", "replace")
    try:
        tables = {r[0] for r in con.execute("SELECT name FROM sqlite_master WHERE type='table'")}

        if src.name == "Cookies" and "cookies" in tables:
            rows = []
            for r in con.execute(
                "SELECT host_key,name,value,encrypted_value,path,expires_utc,is_secure,"
                "is_httponly,creation_utc FROM cookies"
            ):
                host, name, value, blob, path_, expires, secure, httponly, created = r
                if not value and blob:
                    pt = decrypt_blob(blob, keys)
                    if pt is not None:
                        value = strip_host_binding(pt, host).decode("utf-8", "replace")
                rows.append([browser, profile, host, name, value, path_,
                             chrome_time_to_iso(expires), secure, httponly,
                             chrome_time_to_iso(created)])
            export_csv(out_dir / f"{browser}_{profile}_cookies.csv",
                       ["browser", "profile", "host_key", "name", "value", "path",
                        "expires_utc", "is_secure", "is_httponly", "creation_utc"], rows)
            counts["cookies"] = len(rows)

        if src.name == "Login Data" and "logins" in tables:
            rows = []
            for r in con.execute(
                "SELECT origin_url,action_url,username_value,password_value,date_created,"
                "times_used,blacklisted_by_user FROM logins"
            ):
                origin, action, user, blob, created, used, blacklisted = r
                pwd = ""
                if blob:
                    pt = decrypt_blob(blob, keys)
                    if pt is not None:
                        pwd = pt.decode("utf-8", "replace")
                rows.append([browser, profile, origin, action, user, pwd,
                             chrome_time_to_iso(created), used, blacklisted])
            export_csv(out_dir / f"{browser}_{profile}_passwords.csv",
                       ["browser", "profile", "origin_url", "action_url", "username_value",
                        "password", "date_created", "times_used", "blacklisted"], rows)
            counts["passwords"] = len(rows)

        if src.name == "Web Data" and "credit_cards" in tables:
            rows = []
            for r in con.execute(
                "SELECT guid,name_on_card,expiration_month,expiration_year,"
                "card_number_encrypted,date_modified FROM credit_cards"
            ):
                guid, name, mon, year, blob, modified = r
                num = ""
                if blob:
                    pt = decrypt_blob(blob, keys)
                    if pt is not None:
                        num = pt.decode("utf-8", "replace")
                rows.append([browser, profile, guid, name, num,
                             f"{mon:02d}/{year}" if mon and year else "",
                             chrome_time_to_iso(modified)])
            export_csv(out_dir / f"{browser}_{profile}_cards.csv",
                       ["browser", "profile", "guid", "name_on_card", "card_number",
                        "expires", "date_modified"], rows)
            counts["cards"] = len(rows)
    finally:
        con.close()
        shutil.rmtree(tmp.parent, ignore_errors=True)
    return counts


def process_browser(browser_dir: Path, out_root: Path) -> dict:
    browser = browser_dir.name
    keys = load_keys(browser_dir)
    if not keys:
        print(f"[!] {browser_dir}: no key files, skipping")
        return {}
    print(f"[+] {browser}: keys loaded: {', '.join(sorted(keys))}")

    result = {}
    for profile_dir in sorted(p for p in browser_dir.iterdir() if p.is_dir()):
        dbs = [p for p in sorted(profile_dir.glob("*")) if p.name in TARGET_DBS and p.is_file()]
        if not dbs:
            continue
        print(f"[+] {browser}/{profile_dir.name}: {len(dbs)} databases")
        for db in dbs:
            if open(db, "rb").read(16) != b"SQLite format 3\x00":
                dst = out_root / browser / profile_dir.name / db.name
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(db, dst)  # e.g. Bookmarks JSON: nothing to decrypt, copy through
                continue
            dst = out_root / browser / profile_dir.name / db.name
            dst.parent.mkdir(parents=True, exist_ok=True)
            stats = {}
            try:
                decrypt_database(db, dst, keys, stats)
                counts = export_credentials(db, keys, browser, profile_dir.name,
                                            out_root / "exports")
                result[f"{browser}/{profile_dir.name}/{db.name}"] = {
                    "decrypted_fields": stats, "exports": counts}
                detail = ", ".join(f"{k}={v}" for k, v in stats.items()) or "no encrypted fields"
                print(f"    {db.name}: {detail}")
            except sqlite3.DatabaseError as e:
                print(f"    [!] {db.name}: {e}")
    return result


def main() -> None:
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass
    ap = argparse.ArgumentParser(description="Decrypt v10/v20 fields in BrowserExtract sqlite dumps")
    ap.add_argument("input", help="extract folder, e.g. C:\\Users\\x\\Downloads\\tsk_..._BrowserExtract")
    ap.add_argument("-o", "--output", help="output folder (default: <input>_decrypted)")
    args = ap.parse_args()

    in_root = Path(args.input)
    if not in_root.is_dir():
        print(f"[!] not a folder: {in_root}", file=sys.stderr)
        sys.exit(1)
    out_root = Path(args.output) if args.output else in_root.parent / (in_root.name + "_decrypted")
    out_root.mkdir(parents=True, exist_ok=True)
    (out_root / "exports").mkdir(exist_ok=True)

    browser_dirs = [p for p in sorted(in_root.iterdir()) if p.is_dir()
                    and ((p / "key_v10_hex.txt").is_file() or (p / "key_v20_hex.txt").is_file())]

    if not browser_dirs:
        print(f"[!] no browser folder with key_*_hex.txt under {in_root}", file=sys.stderr)
        sys.exit(1)

    report = {}
    for bd in browser_dirs:
        report.update(process_browser(bd, out_root))

    report_file = out_root / "report.json"
    report_file.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    total = sum(sum(v.values()) for r in report.values() for v in [r.get("decrypted_fields", {})])
    print(f"\n[*] done: {len(report)} databases, {total} encrypted fields decrypted")
    print(f"[*] decrypted databases -> {out_root}")
    print(f"[*] csv exports         -> {out_root / 'exports'}")
    print(f"[*] report              -> {report_file}")


if __name__ == "__main__":
    main()
