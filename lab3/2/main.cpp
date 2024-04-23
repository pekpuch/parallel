#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <fstream>
#include <random>

struct Task {
    int id;            // Идентификатор задачи
    int type;          // Тип задачи (1: sin, 2: sqrt, 3: pow)
    double arg;        // Аргумент для вычисления
    double result;     // Результат вычисления
};

template<typename T>
class Server {
private:
    std::queue<Task> taskQueue;   // Очередь задач
    std::vector<Task> results;    // Вектор с результатами вычислений
    std::mutex mtx;               // Мьютекс для синхронизации доступа к общим ресурсам
    std::condition_variable cv;   // Условная переменная для синхронизации потоков
    bool isRunning;               // Флаг, указывающий, работает ли сервер
    std::thread serverThread;     // Поток, в котором выполняется сервер

public:
    void start() {
        serverThread = std::thread(&Server::processTasks, this);
    }

    void stop() {
        isRunning = false;
        cv.notify_all();
        serverThread.join();
    }

    size_t add_task(Task task) {
        std::unique_lock<std::mutex> lock(mtx);
        task.id = results.size();
        taskQueue.push(task);
        cv.notify_one();
        return task.id;
    }

    Task request_result(int id_res) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return results.size() > id_res; });
        return results[id_res];
    }

private:
    void processTasks() {
        while (isRunning) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] { return !taskQueue.empty() || !isRunning; });
                if (!isRunning) break;
                task = taskQueue.front();
                taskQueue.pop();
            }

            if (task.type == 1) {
                task.result = static_cast<T>(std::sin(task.arg));
            } else if (task.type == 2) {
                task.result = static_cast<T>(std::sqrt(task.arg));
            } else if (task.type == 3) {
                task.result = static_cast<T>(std::pow(task.arg, 2));
            }

            {
                std::lock_guard<std::mutex> lock(mtx);
                results.push_back(task);
            }
        }
    }
};

template<typename T>
void client(Server<T>& server, int numTasks, int type) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dist(1, 100);

    for (int i = 0; i < numTasks; ++i) {
        Task task;
        task.type = type;
        task.arg = dist(gen);
        server.add_task(task);
    }
}

int main() {
    Server<double> server;   
    server.start();          
    int N = 10;
    std::thread client1(client<double>, std::ref(server), N, 1);
    std::thread client2(client<double>, std::ref(server), N, 2);
    std::thread client3(client<double>, std::ref(server), N, 3);

    client1.join();
    client2.join();
    client3.join();

    std::ofstream file1("sin_results.txt");
    std::ofstream file2("sqrt_results.txt");
    std::ofstream file3("pow_results.txt");

    for (int i = 0; i < N; ++i) {
        char* cond;
        auto request_result1 = server.request_result(i);
        auto request_result2 = server.request_result(i + N);
        auto request_result3 = server.request_result(i + N*2);
        file1 << "sin(" << request_result1.arg << ") = " << request_result1.result << cond<< std::endl;
        file2 << "sqrt(" << request_result2.arg << ") = " << request_result2.result << std::endl;
        file3 << request_result3.arg << "^2 = " << request_result3.result << std::endl;
    }

    file1.close();
    file2.close();
    file3.close();

    server.stop();   // Остановка сервера
    return 0;
}
