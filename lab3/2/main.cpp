#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <fstream>
#include <random>
#include <unordered_map>

struct Task {
    int id;
    int type; // 1: sin, 2: sqrt, 3: pow
    double arg;
    double result;
};

template<typename T>
T fun_sin(T arg) {
    return std::sin(arg);
}

template<typename T>
T fun_sqrt(T arg) {
    return std::sqrt(arg);
}

template<typename T>
T fun_pow(T arg) {
    return std::pow(arg, 2);
}

template<typename T>
class Server {
private:
    std::queue<Task> taskQueue;
    std::unordered_map<int, Task> results;
    std::mutex mtx;
    std::condition_variable cv;
    bool isRunning = true;
    std::thread serverThread;
    int totalTasks = 0;

public:
    // Запускает поток-обработчик задач
    void start() {
        serverThread = std::thread(&Server::processTasks, this);
        isRunning = true;
    }

    // Останавливает поток-обработчик задач
    void stop() {
        isRunning = false;
        cv.notify_all();  // Уведомляет все ожидающие потоки о том, что сервер останавливается
        serverThread.join();  // Ожидает завершения потока-обработчика
    }

    // Добавляет новую задачу в очередь задач
    size_t add_task(Task task) {
        std::unique_lock<std::mutex> lock(mtx);  // Блокирует mutex для безопасного доступа к очереди задач
        task.id = ++totalTasks;  // Присваивает задаче уникальный идентификатор
        taskQueue.push(task);  // Добавляет задачу в очередь задач
        cv.notify_one();  // Уведомляет один из ожидающих потоков о том, что появилась новая задача
        return task.id;  // Возвращает идентификатор добавленной задачи
    }

    // Возвращает результат выполнения задачи по ее идентификатору
    Task request_result(int id_res) {
        std::unique_lock<std::mutex> lock(mtx);  // Блокирует mutex для безопасного доступа к результатам
        cv.wait(lock, [&] { return results.find(id_res) != results.end(); });  // Ожидает, пока результат не будет готов
        return results[id_res];  // Возвращает результат выполнения задачи
    }

    // Возвращает true, если сервер инициализирован и готов к обработке задач
    bool Run_check() {
        return isRunning;
    }

private:
    // Обрабатывает задачи из очереди задач
    void processTasks() {
        while (isRunning) {  // Цикл обработки задач
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx);  // Блокирует mutex для безопасного доступа к очереди задач
                cv.wait(lock, [&] { return !taskQueue.empty() || !isRunning; });  // Ожидает, пока появится новая задача или сервер будет остановлен
                if (!isRunning) break;  // Выходит из цикла, если сервер был остановлен
                task = taskQueue.front();  // Извлекает первую задачу из очереди задач
                taskQueue.pop();  // Удаляет первую задачу из очереди задач
            }

            // Выполняет задачу в соответствии с ее типом
            if (task.type == 1) {
                task.result = fun_sin<T>(task.arg);
            } else if (task.type == 2) {
                task.result = fun_sqrt<T>(task.arg);
            } else if (task.type == 3) {
                task.result = fun_pow<T>(task.arg);
            }

            // Сохраняет результат выполнения задачи в списке результатов
            {
                std::lock_guard<std::mutex> lock(mtx);  // Блокирует mutex для безопасного доступа к результатам
                results[task.id] = task;  // Сохраняет результат по идентификатору задачи
            }
        }
    }
};

// Шаблонная функция клиента для взаимодействия с сервером
template<typename T>
void client(Server<T>& server, int numTasks, int type) {
    // Создаем генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    // Создаем равномерное распределение в диапазоне [1, 100]
    std::uniform_real_distribution<T> dist(1, 100);

    // Создаем numTasks задач и добавляем их в очередь задач сервера
    for (int i = 0; i < numTasks; ++i) {
        Task task;
        // Устанавливаем тип задачи
        task.type = type;
        // Генерируем случайное значение аргумента задачи
        task.arg = dist(gen);
        // Добавляем задачу в очередь задач сервера
        server.add_task(task);
    }
}

int main() {
    Server<double> server;
    server.start();

    // Ожидаем, пока сервер будет инициализирован и готов к обработке задач
    while (!server.Run_check()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
    std::string cor1;
    std::string cor2;
    std::string cor3;
    for (int i = 0; i < N; ++i) {
        auto request_result1 = server.request_result(i + 1);
        auto request_result2 = server.request_result(i + 11);
        auto request_result3 = server.request_result(i + 21);

        auto res1 = request_result1.arg;
        auto res2 = request_result2.arg;
        auto res3 = request_result3.arg;
        // Проверка
        if (std::abs(std::sin(res1) - request_result1.result) < 1e-9) cor1 = " correct";
        else cor1 = " incorrect";
        if (std::abs(std::sqrt(res2) - request_result2.result) < 1e-9) cor2 = " correct";
        else cor2 = " incorrect";
        if (std::abs(std::pow(res3, 2) - request_result3.result) < 1e-9) cor3 = " correct";
        else cor3 = " incorrect";

        file1 << "sin(" << request_result1.arg << ") = " << request_result1.result << cor1 << std::endl;
        file2 << "sqrt(" << request_result2.arg << ") = " << request_result2.result << cor2 << std::endl;
        file3 << request_result3.arg << "^2 = " << request_result3.result << cor3 << std::endl;
    }

    file1.close();
    file2.close();
    file3.close();

    server.stop();
    return 0;
}
