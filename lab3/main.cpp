#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <omp.h>

void init_matrix(std::vector<std::vector<int>>& matrix, int rows, int cols, int start, int end) {
    std::mutex mtx;
    for (int i = start; i < end; i++) {
        for (int j = 0; j < cols; j++) {
            mtx.lock();
            matrix[i][j] = i + j;
            mtx.unlock();
        }
    }
}

void init_vector(std::vector<int>& vector, int size, int start, int end) {
    std::mutex mtx;
    for (int i = start; i < end; i++) {
        mtx.lock();
        vector[i] = i;
        mtx.unlock();
    }
}

void multiply(std::vector<std::vector<int>>& matrix, std::vector<int>& vector, std::vector<int>& result, int rows, int cols, int start, int end) {
    for (int i = start; i < end; i++) {
        int sum = 0;
        for (int j = 0; j < cols; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

int main() {
    int sizes[2] = {20000, 40000};
    int threads[8] = {1,2,4,7,8,16,20,40};
    int m, n;
    double S;
    int num_threads;
    double final_time;
    double input20[8];
    double input40[8];
    for (int k = 0; k < 2; k++) {
        m = sizes[k];
        n = sizes[k];
        std::cout << "Size = " << n  << std::endl;
        for (int j = 0; j < 8; j++) {
            num_threads = threads[j];
            std::vector<std::vector<int>> matrix(m, std::vector<int>(n));
            std::vector<int> vector(n);
            std::vector<int> result(m);

            std::vector<std::thread> threads(num_threads);

            double start_time = omp_get_wtime();

            int chunk_size = m / num_threads;

            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = (i == num_threads - 1) ? m : start + chunk_size;
                threads[i] = std::thread(init_matrix, std::ref(matrix), m, n, start, end);
            }

            for (auto& t : threads) {
                t.join();
            }

            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = (i == num_threads - 1) ? m : start + chunk_size;
                threads[i] = std::thread(init_vector, std::ref(vector), n, start, end);
            }

            for (auto& t : threads) {
                t.join();
            }

            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = (i == num_threads - 1) ? m : start + chunk_size;
                threads[i] = std::thread(multiply, std::ref(matrix), std::ref(vector), std::ref(result), m, n, start, end);
            }

            for (auto& t : threads) {
                t.join();
            }

            double end_time = omp_get_wtime();
            final_time = end_time - start_time;
            if (num_threads == 1) {
                S = final_time;
                std::cout << "T = " << S << " seconds." << std::endl;
                if (n == 20000) input20[0] = 1;
                else input40[0] = 1;
            }
            else {
                std::cout << "T" << num_threads <<" = " << final_time << " seconds. " << "S" << num_threads << " = " << S/final_time << std::endl;
                if (n == 20000) input20[j] = S/final_time;
                else input40[j] = S/final_time;
            }
        }
    }
    for(int i =0; i < 8; i++) {
        std::cout << input20[i] << ", ";
    }
    std::cout << std::endl;

    for(int i =0; i < 8; i++) {
        std::cout << input40[i] << ", ";
    }
    std::cout << std::endl;


    return 0;
}
