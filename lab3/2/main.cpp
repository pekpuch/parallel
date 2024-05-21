#include <iostream>
#include <queue>
#include <mutex>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <fstream>
#include <random>
#include <iomanip>

template<typename T>
T f_pow(T x, T y)
{
    return std::pow(x, y);
}

template<typename T>
T f_sin(T x)
{
    return std::sin(x);
}

template<typename T>
T f_sqrt(T x)
{
    return std::sqrt(x);
}

// пример сервера обрабатывающего задачи из очереди
template<typename T>
class Server{
public:
    Server(){}
    void start(){
        working = true;
        potok = std::thread(&Server::work,this);
    }

    void stop(){
         working = false;
         potok.join();
    }
    size_t add_task(std::packaged_task<T()> task){
                // добавляем задачу в очередь
                std::unique_lock<std::mutex> lock(mtx);
                std::future<T> result = task.get_future();
                task_idx++;
                size_t idx = task_idx ;
                results[idx] = result.share();
                tasks.push(std::move(task));
                idxs.push(idx);

                //std::cout<<"help";
                return idx;
    }
    T request_result(size_t idx){
        // берем задачу из словаря решаем её и кидаем резы в results
        std::shared_future<T> future;
        {
        std::unique_lock<std::mutex> lock(mtx);
        future = results.find(idx)->second;
        } 
        return future.get();

    }

private:
    void work()
    {
        // пока не получили сигнал стоп
        size_t idx;
        std::packaged_task<T()> task;
        while (working)
        {
            // если очередь не пуста, то достаем задачу и решаем
            std::unique_lock<std::mutex> lock(mtx);
            if(!tasks.empty())
            {
                idx = idxs.front();
                
                task = std::move(tasks.front());
                tasks.pop();
                idxs.pop();
                lock.unlock();
                task();
            }
            else{
                lock.unlock();
                if (!working){
                    break;
                }
            }
        }
        std::cout << "Server stop!\n";
    }

    std::mutex mtx;
    std::thread potok;
    bool working = 0;
    // Очередь задач
    size_t task_idx = 0;
    std::queue<size_t> idxs;
    std::queue<std::packaged_task<T()>> tasks;
    std::unordered_map<size_t,std::shared_future<T>> results;
};

void client(Server<double>& server,int N,int func_type,const std::string& filename)
{
    // для рандомной генерации аргументов
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 10);
    std::uniform_real_distribution<double> dist_real(0.0, 10.0);
    
    // открываем файл
    std::ofstream file(filename);

    for (size_t i = 0; i < N; i++)
    {
        if (func_type==0)
        {
            std::string cor;
            int arg1 = dist(gen);
            int arg2 = dist(gen);
            std::packaged_task<double()> task(std::bind(f_pow<int>,arg1,arg2));

            size_t idx = server.add_task(std::move(task));
            double ans = server.request_result(idx);

            if (std::abs(std::pow(arg1, arg2) - ans) < 1e-9) cor = "correct";
            else cor = "incorrect";

            file << "pow " << arg1 << " " << arg2 <<" = " << ans << " " << cor <<std::endl;
                 
        }
        if (func_type==1)
        {
            std::string cor;
            int arg1 = dist_real(gen);
            std::packaged_task<double()> task(std::bind(f_sin<double>,arg1));
            size_t idx = server.add_task(std::move(task));
            double res = server.request_result(idx);
            if (std::abs(std::sin(arg1) - res) < 1e-9) cor = "correct";
            else cor = "incorrect";

            file << "sin "<< arg1 << " = " << res<< " " << cor << std::endl;
              
        }
        if (func_type==2)
        {
            std::string cor;
            int arg1 = dist_real(gen);
            std::packaged_task<double()> task(std::bind(f_sqrt<double>,arg1));
            size_t idx = server.add_task(std::move(task));
            double res = server.request_result(idx);

            if (std::abs(std::sqrt(arg1) - res) < 1e-9) cor = "correct";
            else cor = "incorrect";

            file << "sqrt "<<arg1<<" = " << res << " " << cor << std::endl;
             
        }
    }
    file.close();
}

int main()
{
    int N = 10000;
    Server<double> server;
    server.start();
    std::thread client_pow(client,std::ref(server),N,0,"pow.txt");
    std::thread client_sin(client,std::ref(server),N,1,"sin.txt");
    std::thread client_sqrt(client,std::ref(server),N,2,"sqrt.txt");

    client_pow.join();
    client_sin.join();
    client_sqrt.join();

    server.stop();
}
