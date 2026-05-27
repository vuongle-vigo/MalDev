"""
Convert string literals in C/C++ char/wchar_t declarations to character arrays.

Usage:
    python convert_string.py <file_or_folder>
"""

import argparse
import os
import re
import sys
from pathlib import Path


# Pattern: match char/wchar_t declarations with string literals.
# Captures: (leading_ws, type_and_name, string_literal)
# Example matches:
#   char msg[] = "hello";
#   wchar_t name[] = L"Hello";
#   const char* s = "test";
#   char data[] = "line\nhere";
RE_PATTERNS = [
    # char/wchar_t name[] = "...";
    re.compile(
        r'^(\s*)'
        r'((?:const\s+)?(?:static\s+)?(?:inline\s+)?(?:constexpr\s+)?)'
        r'(char|wchar_t)\s+(\w+)\s*\[\s*\]\s*=\s*'
        r'((?:u8|u|U|L)?"[^"\\]*(?:\\.[^"\\]*)*")\s*;',
        re.MULTILINE
    ),
    # char/wchar_t name = "...";  (without [])
    re.compile(
        r'^(\s*)'
        r'((?:const\s+)?(?:static\s+)?(?:inline\s+)?(?:constexpr\s+)?)'
        r'(char|wchar_t)\s+(\w+)\s*=\s*'
        r'((?:u8|u|U|L)?"[^"\\]*(?:\\.[^"\\]*)*")\s*;',
        re.MULTILINE
    ),
    # const char* name = "...";
    re.compile(
        r'^(\s*)'
        r'((?:const\s+)?(?:static\s+)?(?:inline\s+)?(?:constexpr\s+)?)'
        r'(char|wchar_t)\s*\*+\s+(\w+)\s*=\s*'
        r'((?:u8|u|U|L)?"[^"\\]*(?:\\.[^"\\]*)*")\s*;',
        re.MULTILINE
    ),
]


def escape_char(ch: str, is_wide: bool) -> str:
    """Escape a single character for C char output."""
    special = {
        '\\': '\\\\',
        "'": "\\'",
        '\n': "\\n",
        '\r': "\\r",
        '\t': "\\t",
        '\0': "\\0",
        '\v': "\\v",
        '\a': "\\a",
        '\b': "\\b",
        '\f': "\\f",
    }
    if ch in special:
        return special[ch]
    code = ord(ch)
    if code < 32 or code > 126:
        return f"\\x{code:02x}"
    return ch


def string_to_char_array(literal: str) -> str:
    """
    Convert a C string literal to a C char-array initializer.

    Examples:
        "hello"        -> {'h', 'e', 'l', 'l', 'o', '\0'}
        L"Hi"          -> {L'H', L'i', L'\0'}
        u8"ok\n"       -> {'o', 'k', '\\n', '\\0'}
    """
    # Determine prefix and inner content
    prefixes = []
    inner = literal

    # Strip prefix
    for p in ('u8', 'u', 'U', 'L'):
        if inner.startswith(p):
            prefixes.append(p)
            inner = inner[len(p):]
            break

    prefix = ''.join(prefixes)
    # Remove surrounding quotes
    if inner.startswith('"') and inner.endswith('"'):
        inner = inner[1:-1]

    # Escape inner backslashes first (before parsing escape sequences)
    inner = inner.replace('\\', '\\\\')

    chars = []
    i = 0
    while i < len(inner):
        if inner[i] == '\\' and i + 1 < len(inner):
            nxt = inner[i + 1]
            escape_seq = '\\' + nxt
            i += 2
            if prefix == 'L':
                chars.append(f"L'{escape_seq}'")
            else:
                chars.append(f"'{escape_seq}'")
        else:
            ch = inner[i]
            escaped = escape_char(ch, prefix == 'L')
            if prefix == 'L':
                chars.append(f"L'{escaped}'")
            else:
                chars.append(f"'{escaped}'")
            i += 1

    # Add null terminator for array declarations
    if prefix == 'L':
        chars.append("L'\\0'")
    else:
        chars.append("'\\0'")

    return '{' + ', '.join(chars) + '}'


def convert_file(filepath: Path, dry_run: bool = False) -> list[dict]:
    """
    Scan a C/C++ file and convert string literals in char/wchar_t declarations.

    Returns a list of change records:
        { line_no, old_line, new_line, decl }
    """
    changes = []

    content = filepath.read_text(encoding='utf-8', errors='replace')
    lines = content.splitlines(keepends=False)

    # Track which lines have already been modified to avoid double-processing
    modified_lines = set()

    for pattern in RE_PATTERNS:
        for m in pattern.finditer(content):
            start_line = content[:m.start()].count('\n')
            end_match = m.group(0)
            end_line = start_line + end_match.count('\n')

            # Skip if any line in the range was already modified
            if any(ln in modified_lines for ln in range(start_line, end_line + 1)):
                continue

            ws = m.group(1)
            modifiers = m.group(2).strip()
            type_name = m.group(3)
            var_name = m.group(4)
            literal = m.group(5)

            # Build new type declaration
            type_part = type_name
            if modifiers:
                type_part = f"{modifiers} {type_name}"

            # Convert string to char array
            char_array = string_to_char_array(literal)
            new_decl = f"{ws}{type_part} {var_name}[] = {char_array};"

            old_text = m.group(0)
            decl_summary = f"{type_name} {var_name}[] = {literal}"

            change = {
                'line_no': start_line + 1,
                'old_text': old_text,
                'new_text': new_decl,
                'var_name': var_name,
                'type_name': type_name,
                'original_literal': literal,
                'char_array': char_array,
                'decl': decl_summary,
            }
            changes.append(change)

            # Mark lines as modified
            for ln in range(start_line, end_line + 1):
                modified_lines.add(ln)

    if dry_run or not changes:
        return changes

    # Apply changes to content
    # Sort by start position descending so replacements don't shift indices
    sorted_changes = sorted(changes, key=lambda c: content.find(c['old_text']), reverse=True)

    new_content = content
    for change in sorted_changes:
        new_content = new_content.replace(change['old_text'], change['new_text'], 1)

    filepath.write_text(new_content, encoding='utf-8')
    return changes


def print_diff(change: dict, filepath: Path):
    """Pretty-print a single change as a diff."""
    print(f"\n{'='*70}")
    print(f"  File  : {filepath}")
    print(f"  Line  : {change['line_no']}")
    print(f"  Decl  : {change['decl']}")
    print(f"{'-'*70}")
    print(f"  OLD: {change['old_text']}")
    print(f"  NEW: {change['new_text']}")


def print_summary(changes_by_file: dict[Path, list], total_changes: int):
    """Print a summary table of all changes."""
    print("\n" + "=" * 70)
    print(f"  SUMMARY: {total_changes} string(s) converted across {len(changes_by_file)} file(s)")
    print("=" * 70)
    print(f"  {'File':<40} {'Count':>6}")
    print("-" * 70)
    for fp, changes in changes_by_file.items():
        rel = fp.relative_to(fp.anchor) if fp.is_absolute() else fp
        print(f"  {str(rel):<40} {len(changes):>6}")
    print("=" * 70)


def find_cpp_files(root: Path) -> list[Path]:
    """Recursively find all .c and .cpp files under root."""
    cpp_exts = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp'}
    files = []
    if root.is_file():
        if root.suffix.lower() in cpp_exts:
            files.append(root)
    else:
        for p in sorted(root.rglob('*')):
            if p.is_file() and p.suffix.lower() in cpp_exts:
                files.append(p)
    return files


def main():
    parser = argparse.ArgumentParser(
        description="Convert char/wchar_t string literals to character arrays.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python convert_string.py my_file.cpp
  python convert_string.py src/folder
  python convert_string.py . --dry-run
  python convert_string.py . --extensions cpp h
        """
    )
    parser.add_argument(
        'path',
        type=Path,
        help="File or folder to process"
    )
    parser.add_argument(
        '--dry-run', '-n',
        action='store_true',
        help="Show changes without modifying files"
    )
    parser.add_argument(
        '--extensions', '-e',
        nargs='+',
        default=['c', 'cpp', 'cc', 'cxx', 'h', 'hpp'],
        metavar='EXT',
        help="File extensions to process (default: c cpp cc cxx h hpp)"
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help="Show verbose output"
    )

    args = parser.parse_args()

    if not args.path.exists():
        print(f"Error: Path does not exist: {args.path}", file=sys.stderr)
        sys.exit(1)

    # Build extension set
    exts = {f'.{e.lstrip(".").lower()}' for e in args.extensions}

    # Find files
    if args.path.is_file():
        files = [args.path]
    else:
        files = [p for p in args.path.rglob('*') if p.is_file() and p.suffix.lower() in exts]

    if not files:
        print(f"No C/C++ files found in: {args.path}")
        sys.exit(0)

    print(f"Scanning {len(files)} file(s)...\n")

    changes_by_file = {}
    total_changes = 0

    for fp in files:
        try:
            changes = convert_file(fp, dry_run=args.dry_run)
            if changes:
                changes_by_file[fp] = changes
                total_changes += len(changes)
                if args.verbose or args.dry_run:
                    for ch in changes:
                        print_diff(ch, fp)
        except Exception as e:
            print(f"  Warning: Could not process {fp}: {e}", file=sys.stderr)

    print_summary(changes_by_file, total_changes)

    if args.dry_run:
        print("\n  [Dry run] No files were modified.")
    else:
        print(f"\n  Done. {total_changes} string(s) converted.")

    sys.exit(0)


if __name__ == '__main__':
    main()
