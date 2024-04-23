import time
import cv2
import argparse
import logging
import queue
import threading
import sys

logging.basicConfig(filename='app.log', level=logging.INFO)
stop_flag = threading.Event()

class Sensor:
    def get(self):
        raise NotImplementedError ("Error")

class SensorX (Sensor):
    def __init__ (self, delay: float):
        self.delay = delay
        self.data = 0
    def get (self) -> int:
        time.sleep(self.delay)
        self.data += 1
        return self.data

class SensorCam(Sensor):
    def __init__(self, camera_name, resolution):
        self.cap = cv2.VideoCapture(camera_name)
        self.cap.set(3, resolution[0])
        self.cap.set(4, resolution[1])
    def get(self):
        ret, frame = self.cap.read()
        return ret, frame
    def release(self):
        self.cap.release()


class WindowImage:
    def __init__(self, freq):
        self.freq = freq
        cv2.namedWindow("window")
    def show(self, img, sensor0, sensor1, sensor2):
        x = 50
        y = 50
        text1 = f"Sensor 1: {sensor0}"
        text2 = f"Sensor 2: {sensor1}"
        text3 = f"Sensor 3: {sensor2}"
        cv2.putText(img, text1, (x, y), cv2.FONT_HERSHEY_SIMPLEX,  0.7, (0, 0, 255), 2)
        cv2.putText(img, text2, (x, y + 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        cv2.putText(img, text3, (x, y + 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        cv2.imshow("window", img)
    def close(self):
        cv2.destroyWindow("window")

def read_sensor(sensor, q):
    while not stop_flag.is_set():
        data = sensor.get()
        q.put(data)

def main (args):
    resolution = (int(args.resolution.split('x')[0]), int(args.resolution.split('x')[1]))
    camera_name = args.camera
    frequency = args.frequency
    
    window = WindowImage(frequency)
    camera = SensorCam(camera_name, resolution)
    
    if not camera.cap.isOpened():
        logging.info('Camera Error')
        camera.release()
        window.close()
        sys.exit()
        
    sensor0 = SensorX (0.01)
    sensor1 = SensorX (0.1)
    sensor2 = SensorX (1)

    q0 = queue.Queue()
    q1 = queue.Queue()
    q2 = queue.Queue()

    
    t0 = threading.Thread(target=read_sensor, args=(sensor0, q0))
    t1 = threading.Thread(target=read_sensor, args=(sensor1, q1))
    t2 = threading.Thread(target=read_sensor, args=(sensor2, q2))
    
    t0.start()
    t1.start()
    t2.start()
    
    while True:
        if not q0.empty():
            sensor0 = q0.get()
        if not q1.empty():
            sensor1 = q1.get()
        if not q2.empty():
            sensor2 = q2.get()

        ret, frame = camera.get()
        if not camera.cap.isOpened() or not camera.cap.grab():
            logging.info('Camera Error')
            camera.release()
            window.close()
            stop_flag.set()
            t0.join()
            t1.join()
            t2.join()
            sys.exit()

        window.show(frame, sensor0, sensor1, sensor2)
        time.sleep(1 / window.freq)
        
        if cv2.waitKey(1) == ord('q'):
            camera.release()
            window.close()
            stop_flag.set()
            t0.join()
            t1.join()
            t2.join()
            sys.exit()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--camera', type=int, default=0, help='Имя камеры в системе (по умолчанию: default)')
    parser.add_argument('-r', '--resolution', type=str, default='1920x1080', help='Желаемое разрешение камеры (ширина и высота)')
    parser.add_argument('-f', '--frequency', type=float, default=30.0, help='Частота отображения картинки (по умолчанию: 30.0)')
    args = parser.parse_args()
    main(args)