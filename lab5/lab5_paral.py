import argparse
import time
import cv2
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from ultralytics import YOLO
import os

# Класс-оболочка для управления ресурсом модели
class ModelResource:
    def __init__(self):
        # Инициализация ресурса (загрузка модели)
        print("Loading model...")
        self.model = YOLO('yolov8s-pose.pt')  # Замените на путь к файлу модели

    def __del__(self):
        # Освобождение ресурса (выгрузка модели)
        print("Unloading model...")
        del self.model

# Функция для обработки одного кадра
def process_frame(frame, model_resource):
    # Извлечение модели из оболочки-ресурса
    model = model_resource.model
    # Инференс модели и наложение KeyPoints на кадр
    results = model.predict(frame, verbose=False)  # Выполнение инференса
    keypoints = results[0].keypoints  # Извлечение KeyPoints
    frame_with_keypoints = results[0].plot()  # Наложение KeyPoints на кадр
    return frame_with_keypoints

# Функция для обработки видео в однопоточном режиме
def process_video_single_threaded(video_path, output_video_path):
    # Измерение времени
    start_time = time.time()

    # Чтение видео
    video = cv2.VideoCapture(video_path)
    fps = video.get(cv2.CAP_PROP_FPS)
    frame_count = int(video.get(cv2.CAP_PROP_FRAME_COUNT))

    # Инициализация оболочки-ресурса модели
    model_resource = ModelResource()

    # Обработка каждого кадра
    for i in range(frame_count):
        ret, frame = video.read()
        if not ret:
            break
        # Изменение разрешения
        frame = cv2.resize(frame, (640, 480))
        processed_frame = process_frame(frame, model_resource)

        # Запись обработанного кадра в выходное видео
        if i == 0:
            fourcc = cv2.VideoWriter_fourcc(*"mp4v")
            out_video = cv2.VideoWriter(output_video_path, fourcc, fps, (640, 480))
        out_video.write(processed_frame)

    # Освобождение ресурсов
    out_video.release()
    video.release()

    # Измерение времени
    end_time = time.time()
    print(f"Single-threaded processing time: {end_time - start_time:.2f} seconds")

# Функция для обработки видео в многопоточном режиме
def process_video_multi_threaded(video_path, output_video_path, num_threads):
    # Измерение времени
    start_time = time.time()

    # Чтение видео
    video = cv2.VideoCapture(video_path)
    fps = video.get(cv2.CAP_PROP_FPS)
    frame_count = int(video.get(cv2.CAP_PROP_FRAME_COUNT))

    # Инициализация оболочки-ресурса модели
    model_resource = ModelResource()

    # Чтение кадров из видео и создание списка фреймов
    frames = []
    for i in range(frame_count):
        ret, frame = video.read()
        if not ret:
            break
        frames.append(frame)

    # Инициализация пула потоков
    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        # Обработка каждого кадра в отдельном потоке
        future_frames = [executor.submit(process_frame, cv2.resize(frame, (640, 480)), model_resource) for frame in frames]

        # Запись обработанного кадра в выходное видео
        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        out_video = cv2.VideoWriter(output_video_path, fourcc, fps, (640, 480))
        for i, future_frame in enumerate(future_frames):
            processed_frame = future_frame.result()
            out_video.write(processed_frame)

    # Освобождение ресурсов
    out_video.release()
    video.release()

    # Измерение времени
    end_time = time.time()
    print(f"Multi-threaded processing time (with {num_threads} threads): {end_time - start_time:.2f} seconds")

# Обработка аргументов командной строки
parser = argparse.ArgumentParser(description="-i - имя видео, -m - режим, -o - имя output, -t - ")
parser.add_argument("-i", help="путь")
parser.add_argument("-m", choices=["single-threaded", "multi-threaded"], help="режим")
parser.add_argument("-o", help="путь output")
parser.add_argument("-t", type=int, default=None, help="число потоков")
args = parser.parse_args()

# Выбор режима обработки видео
if args.m == "single-threaded":
    process_video_single_threaded(args.i, args.o)
elif args.m == "multi-threaded":
    if args.t is None:
        # Автоматическое определение оптимального числа потоков (например, можно использовать число ядер процессора)
        num_threads = os.cpu_count()  # Замените на ваш метод автоопределения
    else:
        num_threads = args.t
    process_video_multi_threaded(args.i, args.o, num_threads)
