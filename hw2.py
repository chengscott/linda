import io
import itertools
import multiprocessing
import os
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
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    frame_tensor = np.zeros((1, 3, frame.shape[0], frame.shape[1]))
    for i in range(3):
        frame_tensor[0, i, :, :] = (frame[:, :, 2 - i] / P_SCALE -
                                    P_MEAN[2 - i]) / P_STD[2 - i]
    cvNet.setInput(frame_tensor)
    output_tensor = cvNet.forward()
    found = any(float(d[2]) > 0.2 for d in output_tensor[0, 0, :, :])
    return found


def recv_size(conn, size, buffer_size=32768):
    data = b''
    while len(data) < size:
        cursize = min(size - len(data), buffer_size)
        data += conn.recv(cursize)
    return data


def handle_video(fname, hname, buffer_size=32768):
    f = open(fname, 'rb')
    fs = pa.hdfs.connect('master', 9000)
    h = fs.open(hname, 'wb', buffer_size, default_block_size=1048576)
    data = f.read(buffer_size)
    while len(data):
        h.write(data)
        h.flush()
        data = f.read(buffer_size)
    f.close()
    h.close()
    print('Write', hname, 'done')


def handle(cid, conn, addr):
    try:
        start_time = time.time()
        print('Connected by', addr)
        name = conn.recv(1024).decode('utf-8')
        fifo_name = f'{cid}.avi'
        os.mkfifo(fifo_name)
        hdfs_writer = multiprocessing.Process(target=handle_video,
                                              args=(fifo_name, name),
                                              daemon=True)
        hdfs_writer.start()
        out = cv2.VideoWriter(fifo_name, cv2.VideoWriter_fourcc(*'mp4v'), 60,
                              (1280, 720))
        print('Create worker', hdfs_writer.name)
        fs = pa.hdfs.connect('master', 9000)
        f = fs.open(f'{os.path.splitext(name)[0]}.txt', 'wb', buffer_size=4, default_block_size=1048576)
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
                f.flush()
            #cv2.imwrite(f'f{c}.jpg', frame)
            out.write(frame)
        f.close()
        out.release()
    except:
        pass
    finally:
        print('Close connection', addr)
        hdfs_writer.join()
        os.unlink(fifo_name)
        conn.close()
        print('Time', time.time() - start_time)


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
                                              args=(i, conn, address))
            #daemon=True)
            process.start()
            print('Start', i, process.name)
    except:
        pass
    finally:
        for process in multiprocessing.active_children():
            print('Shutdown', process.name)
            process.terminate()
            process.join()