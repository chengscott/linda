import itertools
import multiprocessing
import socket
import sys
import time
import numpy as np
import cv2
import pyarrow as pa

P_MEAN = [0.406, 0.456, 0.485]
P_STD = [0.225, 0.224, 0.229]
P_SCALE = 255.0


def detect(frame):
    frame_tensor = np.zeros((1, 3, frame.shape[0], frame.shape[1]))
    for i in range(3):
        frame_tensor[0, i, :, :] = (frame[:, :, 2 - i] / P_SCALE -
                                    P_MEAN[2 - i]) / P_STD[2 - i]
    cvNet.setInput(frame_tensor)
    output_tensor = cvNet.forward()
    found = any(float(d[2]) > 0.4 for d in output_tensor[0, 0, :, :])
    return found


def recv_size(conn, size, buffer_size=4096):
    data = b''
    while len(data) < size:
        cursize = min(size - len(data), buffer_size)
        data += conn.recv(cursize)
    return data


def handle(cid, conn, addr):
    try:
        start_time = time.time()
        print('Connected by', addr)
        name = conn.recv(1024).decode('utf-8')
        out = cv2.VideoWriter(name, cv2.VideoWriter_fourcc(*'mp4v'), 60,
                              (1280, 720))
        fs = pa.hdfs.connect('master', 9000)
        f = fs.open(f'/{cid}.txt', 'wb')
        found = None
        for c in itertools.count():
            data_len = conn.recv(16)
            if not data_len:
                break
            raw_data = recv_size(conn, int(data_len))
            data = np.frombuffer(raw_data, dtype=np.uint8)
            frame = cv2.imdecode(data, cv2.IMREAD_COLOR)
            if c % 10 == 0:
                found = detect(frame)
            if found:
                f.write(f'{c}\n'.encode('utf-8'))
            #cv2.imwrite(f'f{c}.jpg', frame)
            out.write(frame)
        f.close()
        out.release()
    except:
        pass
    finally:
        print('Close connection', addr)
        print('Time', time.time() - start_time)
        conn.close()


if __name__ == '__main__':
    port = int(sys.argv[1])
    socket = socket.socket()
    socket.bind(('', port))
    socket.listen(5)
    # load model
    cvNet = cv2.dnn.readNetFromCaffe('../model.prototxt',
                                     '../model.caffemodel')
    try:
        #while True:
        for i in range(5):
            conn, address = socket.accept()
            process = multiprocessing.Process(target=handle,
                                              args=(i, conn, address),
                                              daemon=True)
            process.start()
            print('Start', i, process.name)
    except:
        pass
    finally:
        for process in multiprocessing.active_children():
            print('Shutdown', process.name)
            process.terminate()
            process.join()