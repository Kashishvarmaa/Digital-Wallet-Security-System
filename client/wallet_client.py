import socket

class WalletClient:
    def __init__(self, host='localhost', port=8080):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self):
        if not self.sock:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))

    def disconnect(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def send_command(self, command):
        try:
            if not self.sock:
                self.connect()
            self.sock.sendall(command.encode())
            response = self.sock.recv(4096).decode()
            return response
        except Exception as e:
            return f"[ERROR] {str(e)}"

    def run(self):
        self.connect()
        try:
            while True:
                cmd = input(">>> ").strip()
                if cmd.lower() in ["exit", "quit"]:
                    break
                response = self.send_command(cmd)
                print(response)
        finally:
            self.disconnect()