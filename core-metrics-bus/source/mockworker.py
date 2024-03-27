import socket
import time

# Connect to localhost:7799 TCP and send bytes 0x99 0x98
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 7799))

body = "name:metric1|description:Description1|"
msg = b"\x99\x98" + len(body).to_bytes(1, byteorder="little") + body.encode("utf-8")
print(msg)
s.sendall(msg)

body = "metric1:10|undefined_metric:12|"

msg = b"\x99\x99\x02" + len(body).to_bytes(2, byteorder="little") + body.encode("utf-8")
print(msg)
s.sendall(msg)

time.sleep(5)
s.close()
