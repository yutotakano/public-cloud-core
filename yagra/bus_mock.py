import socketserver


class BusMock(socketserver.BaseRequestHandler):
    def handle(self):
        self.data = self.request.recv(1024).strip()
        print(f"{self.client_address[0]} wrote: {self.data}")


if __name__ == "__main__":
    with socketserver.TCPServer(("localhost", 1234), BusMock) as server:
        server.serve_forever()
