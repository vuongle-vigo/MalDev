import argparse
import base64


def convert_file_to_base64(input_path: str, output_path: str) -> None:
    """Read a file in binary mode, encode its contents to Base64, and write result to output_path."""
    with open(input_path, "rb") as input_file:
        data = input_file.read()

    b64_data = base64.b64encode(data).decode("ascii")

    with open(output_path, "w", encoding="utf-8") as output_file:
        output_file.write(b64_data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert a file to Base64 and save the result to an output file."
    )
    parser.add_argument("input_file", help="Path to the input file to encode")
    parser.add_argument("output_file", help="Path to the output file where Base64 text will be written")
    args = parser.parse_args()

    convert_file_to_base64(args.input_file, args.output_file)
