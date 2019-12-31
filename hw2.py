import itertools
import multiprocessing
import socket
import sys
import numpy as np
import cv2


def recv_size(conn, size, buffer_size=4096):
    data = b''
    while len(data) < size:
        cursize = min(size - len(data), buffer_size)
        data += conn.recv(cursize)
    return data


def handle(conn, addr):
    try:
        print('Connected by', addr)
        name = conn.recv(1024).decode('utf-8')
        out = cv2.VideoWriter(name, cv2.VideoWriter_fourcc(*'mp4v'), 60,
                              (1280, 720))
        for c in itertools.count():
            data_len = conn.recv(16)
            if not data_len:
                break
            raw_data = recv_size(conn, int(data_len))
            data = np.frombuffer(raw_data, dtype=np.uint8)
            frame = cv2.imdecode(data, cv2.IMREAD_COLOR)
            #cv2.imwrite(f'f{c}.jpg', frame)
            out.write(frame)
        out.release()
    except:
        pass
    finally:
        print('Close connection', addr)
        conn.close()


if __name__ == '__main__':
    port = int(sys.argv[1])
    socket = socket.socket()
    socket.bind(('', port))
    socket.listen(5)

    try:
        while True:
            conn, address = socket.accept()
            process = multiprocessing.Process(target=handle,
                                              args=(conn, address))
            process.daemon = True
            process.start()
            print('Start', process.name)
    except:
        pass
    finally:
        for process in multiprocessing.active_children():
            print('Shutdown', process.name)
            process.terminate()
            process.join()