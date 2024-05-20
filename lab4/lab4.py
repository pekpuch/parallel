
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
        self.img = None
        self.sensor0_value = 0
        self.sensor1_value = 0
        self.sensor2_value = 0
        cv2.namedWindow("window")
    def show(self, q0, q1, q2, q_cam):
        if not q_cam.empty():
            self.img = q_cam.get()
            if not q0.empty():
                self.sensor0_value = q0.get()
            if not q1.empty():
                self.sensor1_value = q1.get()
            if not q2.empty():
                self.sensor2_value = q2.get()
            x = 50
            y = 50
            text1 = f"Sensor 1: {self.sensor0_value}"
            text2 = f"Sensor 2: {self.sensor1_value}"
            text3 = f"Sensor 3: {self.sensor2_value}"
            cv2.putText(self.img, text1, (x, y), cv2.FONT_HERSHEY_SIMPLEX,  0.7, (0, 0, 255), 2)
            cv2.putText(self.img, text2, (x, y + 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.putText(self.img, text3, (x, y + 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.imshow("window", self.img)
            
    def close(self):
        cv2.destroyWindow("window")

def read_sensor(sensor, q):
    while not stop_flag.is_set():
        a = sensor.get()
        if q.full():
            q.get()
            q.put(a)
        else:
            q.put(a)
        
def read_camera(camera, q):
    while not stop_flag.is_set():
        _, a = camera.get()
        if q.full():
            q.get()
            q.put(a)
        else:
            q.put(a)


def main (args):
    if 'x' in args.resolution:
        resolution = tuple(map(int, args.resolution.split('x')))
    else:
        logging.error('Wrong res')
        sys.exit()
        
    frequency = args.frequency
    
    if (isinstance(frequency,('float')) == False:
        logging.error('Wrong freq')
        sys.exit()
    
    try:
        camera = SensorCam(args.camera, resolution)
    except Exception as e:
        logging.error('Camera Name Error')
        sys.exit()

    
    if not camera.cap.isOpened():
        logging.error('Camera Error')
        camera.release()
        window.close()
        sys.exit()
        
    name = args.camera
    
    sensor0 = SensorX (0.01)
    sensor1 = SensorX (0.1)
    sensor2 = SensorX (1)
    camera = SensorCam(name, resolution)
    
    q0 = queue.Queue(1)
    q1 = queue.Queue(1)
    q2 = queue.Queue(1)
    camera_q = queue.Queue(1)
    
    t_cam = threading.Thread(target=read_camera, args=(camera, camera_q))
    t0 = threading.Thread(target=read_sensor, args=(sensor0, q0))
    t1 = threading.Thread(target=read_sensor, args=(sensor1, q1))
    t2 = threading.Thread(target=read_sensor, args=(sensor2, q2))

    window = WindowImage(frequency)
    
    t0.start()
    t1.start()
    t2.start()
    t_cam.start()
    
    while True:
        window.show(q0, q1, q2, camera_q)        
        if cv2.waitKey(1) == ord('q'):
            camera.release()
            window.close()
            stop_flag.set()
            t0.join()
            t1.join()
            t2.join()
            t_cam.join()
            break
    time.sleep(1 / frequency)
    sys.exit()     
            

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--camera', type=int, default=0, help='Имя камеры в системе (по умолчанию: default)')
    parser.add_argument('-r', '--resolution', type=str, default='1920x1080', help='Желаемое разрешение камеры (ширина и высота)')
    parser.add_argument('-f', '--frequency', type=float, default=30.0, help='Частота отображения картинки (по умолчанию: 30.0)')
    args = parser.parse_args()
    main(args)
