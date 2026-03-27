import sys
from pathlib import Path

def clear_segments(input_file):
    """
    Remove all lines between pdata SEGMENT and pdata ENDS (inclusive)
    Remove all lines between xdata SEGMENT and xdata ENDS (inclusive)
    Ghi đè lại file cũ
    """
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Lỗi khi đọc file: {e}")
        return False
    
    filtered_lines = []
    skip = False
    segments_to_remove = ['pdata', 'xdata', 'voltbl']
    
    for line in lines:
        # Thay thế 'gs:96' thành 'gs:[96]'
        line = line.replace('gs:96', 'gs:[96]')
        
        # Bắt đầu bỏ qua khi gặp SEGMENT
        for segment in segments_to_remove:
            if f'{segment}\tSEGMENT' in line or f'{segment} SEGMENT' in line:
                skip = True
                break
        
        # Nếu đang bỏ qua, không thêm dòng vào kết quả
        if not skip:
            filtered_lines.append(line)
        
        # Dừng bỏ qua khi gặp ENDS
        for segment in segments_to_remove:
            if f'{segment}\tENDS' in line or f'{segment} ENDS' in line:
                skip = False
                break
    
    # Ghi đè lại file cũ
    try:
        with open(input_file, 'w', encoding='utf-8') as f:
            f.writelines(filtered_lines)
        print(f"✓ Hoàn thành! File đã được cập nhật: {input_file}")
        return True
    except Exception as e:
        print(f"Lỗi khi ghi file: {e}")
        return False


def process_input_path(input_path):
    path = Path(input_path)

    if not path.exists():
        print(f"Lỗi: Không tìm thấy đường dẫn: {input_path}")
        return False

    if path.is_file():
        if path.suffix.lower() != '.asm':
            print(f"Lỗi: File không phải .asm: {input_path}")
            return False
        return clear_segments(str(path))

    asm_files = [p for p in path.rglob('*') if p.is_file() and p.suffix.lower() == '.asm']

    if not asm_files:
        print(f"Không tìm thấy file .asm nào trong folder: {input_path}")
        return False

    print(f"Tìm thấy {len(asm_files)} file .asm. Bắt đầu xử lý...")
    success_count = 0

    for asm_file in asm_files:
        if clear_segments(str(asm_file)):
            success_count += 1

    failed_count = len(asm_files) - success_count
    print(f"\nTổng kết: thành công {success_count}/{len(asm_files)}, thất bại {failed_count}")
    return failed_count == 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Cách dùng: python clear_temp_segment.py <input_file_hoặc_folder>")
        print("\nVí dụ:")
        print("  python clear_temp_segment.py input.asm")
        print("  python clear_temp_segment.py .\\asm_folder")
        sys.exit(1)
    
    input_path = sys.argv[1]
    process_input_path(input_path)