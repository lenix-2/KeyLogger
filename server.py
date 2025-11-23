from http.server import HTTPServer, BaseHTTPRequestHandler
import datetime
import json
import hashlib
import threading


class SecureLogHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        try:
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)

            # Логируем с хэшем для идентификации
            client_hash = hashlib.md5(self.headers.get('User-Agent', '').encode() +
                                      self.client_address[0].encode()).hexdigest()[:8]

            timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
            filename = f"logs/log_{client_hash}_{timestamp}.txt"

            with open(filename, "w", encoding="utf-8") as f:
                f.write(post_data.decode('utf-8', errors='ignore'))

            print(f"Log saved: {filename} from {self.client_address[0]}")

            self.send_response(200)
            self.end_headers()

        except Exception as e:
            print(f"Error: {e}")
            self.send_response(500)
            self.end_headers()

    def log_message(self, format, *args):
        return  # Полностью отключаем логирование


def run_server(port=8080):
    import os
    os.makedirs("logs", exist_ok=True)

    server = HTTPServer(('0.0.0.0', port), SecureLogHandler)
    print(f"Secure server listening on port {port}")
    server.serve_forever()


if __name__ == "__main__":
    run_server()