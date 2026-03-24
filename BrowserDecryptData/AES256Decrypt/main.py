import sys
import sqlite3
from Crypto.Cipher import AES
import base64
import json  # Thêm thư viện json để ghi dữ liệu vào file JSON

if len(sys.argv) != 3:
    print("Usage: py main.py <cookies_db> <key_path>")
    sys.exit(1)

cookies_db = sys.argv[1]
key_path = sys.argv[2]

# Đọc khóa từ file
with open(key_path, 'rb') as f:
    key = f.read().strip()

key = key[:32]  # Đảm bảo khóa có độ dài 32 byte cho AES-256

# Kết nối tới cơ sở dữ liệu SQLite
conn = sqlite3.connect(cookies_db)
cursor = conn.cursor()

cursor.execute("SELECT * FROM cookies")
columns = [description[0] for description in cursor.description]
rows = cursor.fetchall()

# Mở file JSON để ghi
with open('decrypted_cookies.json', 'w') as json_file:
    # Khởi tạo danh sách để chứa tất cả các đối tượng JSON
    all_cookies = []

    for row in rows:
        row_dict = dict(zip(columns, row))
        encrypted_value = row_dict['encrypted_value']
        if encrypted_value.startswith(b'v10'):
            encrypted_value = encrypted_value[3:]
            tag = encrypted_value[-16:]
            cipher = AES.new(key, AES.MODE_GCM, nonce=encrypted_value[:12])

            # Giải mã dữ liệu và xác minh tag
            try:
                decrypted_value = cipher.decrypt_and_verify(encrypted_value[12:-16], tag)
                print(f"Decrypted value for {row_dict['name']}: {decrypted_value.decode('utf-8')}")

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
                    "value": decrypted_value.decode('utf-8')
                }

                # Thêm đối tượng vào danh sách all_cookies
                all_cookies.append(decrypted_data)
            except ValueError as e:
                print(f"Error decrypting value for {row_dict['name']}: {e}")
        else:
            print(f"Value for {row_dict['name']} is not encrypted.")

    # Ghi tất cả các đối tượng vào file JSON dưới dạng một mảng
    json.dump(all_cookies, json_file, indent=4)

# Đóng kết nối cơ sở dữ liệu
conn.close()