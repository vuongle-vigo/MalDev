import sys
import re

def extract_string(input_text):
    # Hỗ trợ đầu vào kiểu: const char* original = "Hello, World!";
    match = re.search(r'^\s*const\s+char\s*\*?\s*\w+\s*=\s*"([^"]*)";\s*$', input_text)
    if match:
        return match.group(1)
    return input_text


def convert_to_c_array(input_text):
    string = extract_string(input_text)
    return '{' + ', '.join(f"'{c}'" for c in string) + '}'

if len(sys.argv) != 2:
    print("Usage: python variable2stack.py \"<input_string>\"")
    sys.exit(1)

input_text = sys.argv[1]
print(convert_to_c_array(input_text))