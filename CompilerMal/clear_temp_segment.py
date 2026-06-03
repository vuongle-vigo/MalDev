import sys
from pathlib import Path

sys.stdout.reconfigure(encoding='utf-8')
sys.stderr.reconfigure(encoding='utf-8')

def clear_segments(input_file):
    """
    Remove all lines between pdata SEGMENT and pdata ENDS (inclusive)
    Remove all lines between xdata SEGMENT and xdata ENDS (inclusive)
    Overwrite the original file.
    """
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading file: {e}")
        return False
    
    filtered_lines = []
    skip = False
    segments_to_remove = ['pdata', 'xdata', 'voltbl']
    assume_fs_nothing_inserted = False
    
    for line in lines:
        # Replace 'gs:96' with 'gs:[96]'
        line = line.replace('gs:96', 'gs:[96]')
        line = line.replace('fs:48', 'fs:[48]')
        # Remove FLAT prefix from OFFSET in ASM operands
        line = line.replace('OFFSET FLAT:', 'OFFSET ')

        # Insert ASSUME FS:NOTHING before the first line containing fs:48
        if not assume_fs_nothing_inserted and 'fs:[48]' in line:
            filtered_lines.append('\tASSUME FS:NOTHING\n')
            assume_fs_nothing_inserted = True

        # Start skipping when hitting SEGMENT
        for segment in segments_to_remove:
            if f'{segment}\tSEGMENT' in line or f'{segment} SEGMENT' in line:
                skip = True
                break
        
        # If skipping, do not add the line to output
        if not skip:
            filtered_lines.append(line)
        
        # Stop skipping when hitting ENDS
        for segment in segments_to_remove:
            if f'{segment}\tENDS' in line or f'{segment} ENDS' in line:
                skip = False
                break
    
    # Overwrite the original file
    try:
        with open(input_file, 'w', encoding='utf-8') as f:
            f.writelines(filtered_lines)
        print(f"[OK] Done! File updated: {input_file}")
        return True
    except Exception as e:
        print(f"Error writing file: {e}")
        return False


def process_input_path(input_path):
    path = Path(input_path)

    if not path.exists():
        print(f"Error: Path not found: {input_path}")
        return False

    if path.is_file():
        if path.suffix.lower() != '.asm':
            print(f"Error: Not a .asm file: {input_path}")
            return False
        return clear_segments(str(path))

    asm_files = [p for p in path.rglob('*') if p.is_file() and p.suffix.lower() == '.asm']

    if not asm_files:
        print(f"No .asm files found in folder: {input_path}")
        return False

    print(f"Found {len(asm_files)} .asm file(s). Starting process...")
    success_count = 0

    for asm_file in asm_files:
        if clear_segments(str(asm_file)):
            success_count += 1

    failed_count = len(asm_files) - success_count
    print(f"\nSummary: success {success_count}/{len(asm_files)}, failed {failed_count}")
    return failed_count == 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python clear_temp_segment.py <input_file_or_folder>")
        print("\nExamples:")
        print("  python clear_temp_segment.py input.asm")
        print("  python clear_temp_segment.py .\\asm_folder")
        sys.exit(1)
    
    input_path = sys.argv[1]
    process_input_path(input_path)