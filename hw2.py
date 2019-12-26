import socket
import sys
import numpy as np
import cv2

def recv_size(conn, size, buffer_size = 4096):
    data = b''
    while len(data) < size:
        data += conn.recv(buffer_size)
    return data

port = int(sys.argv[1])

sock = socket.socket()
sock.bind(('', port))
sock.listen(5)

conn, addr = sock.accept()
print('Connected by', addr)
name = conn.recv(1024).decode('utf-8')
out = cv2.VideoWriter(name, cv2.VideoWriter_fourcc(*'mp4v'), 60,
                      (1280, 720))
while True:
    data_len = conn.recv(16)
    if not data_len:
        break
    raw_data = recv_size(conn, int(data_len))
    data = np.frombuffer(raw_data, dtype=np.uint8)
    frame = cv2.imdecode(data, cv2.IMREAD_COLOR)
    #cv2.imwrite('t.jpg', frame)
    out.write(frame)
conn.close()
out.release()