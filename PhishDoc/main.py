
import argparse
import os
import sys
import base64
from docx import Document
import pyperclip


def read_data(path: str, binary: bool = False, encoding: str = "utf-8") -> str:
    if binary:
        with open(path, "rb") as f:
            return base64.b64encode(f.read()).decode("ascii")

    with open(path, "r", encoding=encoding) as f:
        return f.read()


def main():
    parser = argparse.ArgumentParser(
        description="Tạo file DOCX và ghi nội dung từ file input bắt đầu từ trang chỉ định."
    )

    parser.add_argument("data_file", help="Đường dẫn file data input")

    parser.add_argument(
        "-o", "--output",
        default="output.docx",
        help="Đường dẫn file DOCX đầu ra"
    )

    parser.add_argument(
        "-p", "--page",
        type=int,
        default=20,
        help="Trang bắt đầu ghi dữ liệu, mặc định là 20"
    )

    parser.add_argument(
        "--binary",
        action="store_true",
        help="Đọc file nhị phân và chuyển thành Base64"
    )

    parser.add_argument(
        "--encoding",
        default="utf-8",
        help="Encoding dùng khi đọc file text, mặc định utf-8"
    )

    args = parser.parse_args()

    data_file = os.path.abspath(args.data_file)
    output_file = os.path.abspath(args.output)

    if not os.path.isfile(data_file):
        print(f"[!] Không tìm thấy file input: {data_file}", file=sys.stderr)
        sys.exit(1)

    if args.page < 1:
        print("[!] Số trang phải >= 1", file=sys.stderr)
        sys.exit(1)

    try:
        content = read_data(
            path=data_file,
            binary=args.binary,
            encoding=args.encoding
        )

        doc = Document()

        # Tạo các trang trống đến trước trang đích
        for _ in range(args.page - 1):
            doc.add_page_break()

        # Thêm tiền tố
        content = "abcs:" + content

        # Copy nội dung vào clipboard
        pyperclip.copy(content)

        # Ghi nội dung vào DOCX
        doc.add_paragraph(content)

        # Lưu file
        doc.save(output_file)

        print(f"[+] Đã tạo file: {output_file}")
        print(f"[+] Đã ghi dữ liệu bắt đầu từ trang {args.page}")
        print("[+] Đã copy dữ liệu vào clipboard")

    except UnicodeDecodeError as e:
        print(
            f"[!] Không đọc được file text với encoding '{args.encoding}': {e}",
            file=sys.stderr
        )
        print(
            "[i] Nếu đây là file nhị phân, hãy chạy thêm tùy chọn --binary",
            file=sys.stderr
        )
        sys.exit(2)

    except Exception as e:
        print(f"[!] Lỗi: {e}", file=sys.stderr)
        sys.exit(3)


if __name__ == "__main__":
    main()