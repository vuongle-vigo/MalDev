import sys
import sqlite3
from Crypto.Cipher import AES
import json  # Thêm thư viện json để ghi dữ liệu vào file JSON


def decode_cookie_value(raw_value):
    """Decode decrypted cookie bytes to text, handling Chromium v20 metadata prefix."""
    # Chromium v20 may prepend a 32-byte binary prefix before UTF-8 cookie text.
    candidates = [raw_value]
    if len(raw_value) > 32:
        candidates.append(raw_value[32:])

    for candidate in candidates:
        try:
            return candidate.decode('utf-8'), True
        except UnicodeDecodeError:
            pass

    return raw_value.hex(), False

if len(sys.argv) != 3:
    print("Usage: py main.py <cookies_db> <key_path>")
    sys.exit(1)

cookies_db = sys.argv[1]
key_path = sys.argv[2]

# Đọc khóa từ file
with open(key_path, 'rb') as f:
    key = f.read().strip()

key = key[:32]  # Đảm bảo khóa có độ dài 32 byte cho AES-256
print(f"Key read from file: {key.hex()}")
# Kết nối tới cơ sở dữ liệu SQLite
conn = sqlite3.connect(cookies_db)
cursor = conn.cursor()

# Force encrypted_value to be returned as raw bytes.
cursor.execute("""
    SELECT
        host_key,
        name,
        path,
        expires_utc,
        is_httponly,
        is_secure,
        CAST(encrypted_value AS BLOB) AS encrypted_value
    FROM cookies
""")
columns = [description[0] for description in cursor.description]
rows = cursor.fetchall()

# Mở file JSON để ghi
with open('decrypted_cookies.json', 'w') as json_file:
    # Khởi tạo danh sách để chứa tất cả các đối tượng JSON
    all_cookies = []

    for row in rows:
        row_dict = dict(zip(columns, row))
        encrypted_value = row_dict['encrypted_value']
        if encrypted_value.startswith((b'v10', b'v11', b'v20')):
            encrypted_value = encrypted_value[3:]
            tag = encrypted_value[-16:]
            cipher = AES.new(key, AES.MODE_GCM, nonce=encrypted_value[:12])

            # Giải mã dữ liệu và xác minh tag
            try:
                decrypted_raw = cipher.decrypt_and_verify(encrypted_value[12:-16], tag)
                decoded_value, is_utf8 = decode_cookie_value(decrypted_raw)

                if is_utf8:
                    print(f"Decrypted value for {row_dict['name']}: {decoded_value}")
                else:
                    print(f"Decrypted value for {row_dict['name']} is binary; exporting as hex.")

                # Tạo đối tượng dữ liệu cần ghi vào JSON
                decrypted_data = {
                    "domain": row_dict["host_key"],
                    "expirationDate": row_dict["expires_utc"],
                    "hostOnly": row_dict["host_key"].startswith('.'),
                    "httpOnly": bool(row_dict["is_httponly"]),
                    "secure": bool(row_dict["is_secure"]),
                    "session": False,  
                    "name": row_dict["name"],
                    "path": row_dict["path"],
                    "expires_utc": row_dict["expires_utc"],
                    "value": decoded_value
                }

                # Thêm đối tượng vào danh sách all_cookies
                all_cookies.append(decrypted_data)
            except (ValueError, UnicodeDecodeError) as e:
                print(f"Error decrypting value for {row_dict['name']}: {e}")
        else:
            print(f"Value for {row_dict['name']} is not encrypted.")

    # Ghi tất cả các đối tượng vào file JSON dưới dạng một mảng
    json.dump(all_cookies, json_file, indent=4)

# Đóng kết nối cơ sở dữ liệu
conn.close()