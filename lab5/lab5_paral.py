import argparse
import time
import cv2
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from ultralytics import YOLO
import os
import math
import threading
import queue
import torch

class ModelResource:
    def __init__(self):
        self.model = YOLO('yolov8s-pose.pt')  
    def __del__(self):
        del self.model

def batch_work (batch, model_resource, q):
    model = model_resource.model
    model.to(torch.device('cpu'))
    for frame in batch:
        results = model.predict(frame, verbose=False)
        keypoints = results[0].keypoints
        frame_with_keypoints = results[0].plot()
        q.put(frame_with_keypoints)
    

def video_pred (video_path, output_path, num_threads):
    video = cv2.VideoCapture(video_path)
    fps = 12.88
    num_frames = int(video.get(cv2.CAP_PROP_FRAME_COUNT)) #21
    batch_size = math.ceil(num_frames / num_threads) #6
    frame_size = (640, 480)

    # Разбиение кадров видео на батчи
    batch = []
    batches_list =[]
    i = 0 
    while (i < num_frames):
        for j in range (batch_size):
            if (i == num_frames):
                break
            ret, frame = video.read()
            frame = cv2.resize(frame, (640, 480))
            batch.append(frame)
            i+=1
        batches_list.append(batch)
        batch = []
        
    # Создание очередей и потоков
    threads_list = []
    queue_list = []
    model_list = []
    thread_idx = 0
    for batch in batches_list:
        q = queue.Queue(len(batch))
        queue_list.append(q)
        model_resource = ModelResource()
        model_list.append(model_resource)
        thread = threading.Thread(target=batch_work, args=(batch, model_list[thread_idx], queue_list[thread_idx]))
        thread_idx+=1
        threads_list.append(thread)

    # Замер времени
    start_time = time.time()
    
    # Запуск потоков
    for thread in threads_list:
        thread.start()

    # Остановка потоков
    for thread in threads_list:
        thread.join()
        
    # Окончание замера времени
    total_time = time.time() - start_time

    # Объединение очередей
    frames_list = []
    for q in queue_list:
        while not q.empty():
            frames_list.append(q.get())

    # Создание видео
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')  
    out = cv2.VideoWriter(output_path, fourcc, fps, frame_size)  
    for frame in frames_list: 
        out.write(frame)
    out.release()
    return total_time
    
def main (args):
    video_path = args.input
    output_path = args.output
    num_threads = args.num_threads
    
    total_time = video_pred (video_path, output_path, num_threads)
    
    print(f"для {num_threads} потоков T = {total_time:.4f}")
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', type=str, default='video.mp4', help='Путь до видео (по умолчанию: video.mp4)')
    parser.add_argument('-t', '--num_threads', type=int, default=1, help='Число потоков (по умолчанию: 1)')
    parser.add_argument('-o', '--output', type=str, default='output.mp4', help='Путь до обработанного видео (по умолчанию: output.mp4)')
    args = parser.parse_args()
    main(args)
