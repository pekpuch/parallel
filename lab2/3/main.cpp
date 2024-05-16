#include <iostream>
#include <omp.h>
#include <vector>
#include <cmath>

void initialize_matrix(std::vector<std::vector<double>> &A, std::vector<double> &b, int n) {
    // Заполнение матрицы A и вектора b
    for (int i = 0; i < n; i++) {
        b[i] = i + 1;
        for (int j = 0; j < n; j++) {
            if (i == j)
                A[i][j] = 2.0;
            else
                A[i][j] = 1.0;
        }
    }
}

std::vector<double> jacobi_method_parallel1(const std::vector<std::vector<double>> &A, const std::vector<double> &b, int n, int max_iter, double tol) {
    std::vector<double> x(n, 0.0), x_old(n, 0.0);
    for (int iter = 0; iter < max_iter; iter++) {
        #pragma omp parallel for
        for (int i = 0; i < n; i++) {
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i)
                    sigma += A[i][j] * x_old[j];
            }
            x[i] = (b[i] - sigma) / A[i][i];
        }
        double error = 0.0;
        #pragma omp parallel for reduction(+:error)
        for (int i = 0; i < n; i++) {
            error += abs(x[i] - x_old[i]);
        }
        if (error < tol)
            break;
        x_old = x;
    }
    return x;
}

std::vector<double> jacobi_method_parallel2(const std::vector<std::vector<double>> &A, const std::vector<double> &b, int n, int max_iter, double tol) {
    std::vector<double> x(n, 0.0), x_old(n, 0.0);
    #pragma omp parallel
    {
        for (int iter = 0; iter < max_iter; iter++) {
            #pragma omp for
            for (int i = 0; i < n; i++) {
                double sigma = 0.0;
                for (int j = 0; j < n; j++) {
                    if (j != i)
                        sigma += A[i][j] * x_old[j];
                }
                x[i] = (b[i] - sigma) / A[i][i];
            }
            double error = 0.0;
            #pragma omp for reduction(+:error)
            for (int i = 0; i < n; i++) {
                error += abs(x[i] - x_old[i]);
            }
            if (error < tol)
                break;
            #pragma omp single
            x_old = x;
        }
    }
    return x;
}

std::vector<double> jacobi_method_schedule(const std::vector<std::vector<double>> &A, const std::vector<double> &b, int n, int max_iter, double tol, const std::string& schedule_type) {
    std::vector<double> x(n, 0.0), x_old(n, 0.0);
    omp_sched_t schedule;
    if (schedule_type == "static")
        schedule = omp_sched_static;
    else if (schedule_type == "dynamic")
        schedule = omp_sched_dynamic;
    else
        schedule = omp_sched_guided;

    #pragma omp parallel
    {
        omp_set_schedule(schedule, 1);
        for (int iter = 0; iter < max_iter; iter++) {
            #pragma omp for schedule(runtime)
            for (int i = 0; i < n; i++) {
                double sigma = 0.0;
                for (int j = 0; j < n; j++) {
                    if (j != i)
                        sigma += A[i][j] * x_old[j];
                }
                x[i] = (b[i] - sigma) / A[i][i];
            }
            double error = 0.0;
            #pragma omp for reduction(+:error)
            for (int i = 0; i < n; i++) {
                error += abs(x[i] - x_old[i]);
            }
            if (error < tol)
                break;
            #pragma omp single
            x_old = x;
        }
    }
    return x;
}

int main() {
    int n = 40000; // Размер системы
    int max_iter = 1000;
    double tol = 1e-6;
    int threads[8] = {1, 2, 4, 7, 8, 16, 20, 40};
    double start;
    double end;
    std::vector<double> x;
    std::vector<std::vector<double>> A(n, std::vector<double>(n, 0.0));
    std::vector<double> b(n, 0.0);
    
    initialize_matrix(A, b, n);
    std::cout << "Вариант 1" <<std::endl;
    omp_set_num_threads(1);
    start = omp_get_wtime();
    x = jacobi_method_parallel1(A, b, n, max_iter, tol);
    end = omp_get_wtime();
    std::cout << "T" << 1 <<" = " << end - start << std::endl;
    double T1 = end - start;

    //первый вариант
    for (int i = 1; i < 8; i++) {
        omp_set_num_threads(threads[i]);
        start = omp_get_wtime();
        x = jacobi_method_parallel1(A, b, n, max_iter, tol);
        end = omp_get_wtime();
        std::cout << "T" << threads[i] <<" = " << end - start << ", S" << threads[i] << " = " << T1/(end - start) << std::endl;
    }
    std::cout << "Вариант 2" <<std::endl;
    omp_set_num_threads(1);
    start = omp_get_wtime();
    x = jacobi_method_parallel2(A, b, n, max_iter, tol);
    end = omp_get_wtime();
    std::cout << "T" << 1 <<" = " << end - start << std::endl;
    T1 = end - start;

    //второй вариант
    for (int i = 1; i < 8; i++) {
        omp_set_num_threads(threads[i]);
        start = omp_get_wtime();
        x = jacobi_method_parallel2(A, b, n, max_iter, tol);
        end = omp_get_wtime();
        std::cout << "T" << threads[i] <<" = " << end - start << ", S" << threads[i] << " = " << T1/(end - start) << std::endl;
    }

    //тесты schedule
    std::cout << "тесты schedule\n" <<std::endl;
    std::cout << "static" <<std::endl;
    //static
    omp_set_num_threads(1);
    start = omp_get_wtime();
    x = jacobi_method_schedule(A, b, n, max_iter, tol, "static");
    end = omp_get_wtime();
    std::cout << "T" << 1 <<" = " << end - start << std::endl;
    T1 = end - start;
        for (int i = 1; i < 8; i++) {
        omp_set_num_threads(threads[i]);
        start = omp_get_wtime();
        x = jacobi_method_schedule(A, b, n, max_iter, tol, "static");
        end = omp_get_wtime();
        std::cout << "T" << threads[i] <<" = " << end - start << ", S" << threads[i] << " = " << T1/(end - start) << std::endl;
    }

    std::cout << "dynamic" <<std::endl;
    //dynamic
    omp_set_num_threads(1);
    start = omp_get_wtime();
    x = jacobi_method_schedule(A, b, n, max_iter, tol, "dynamic");
    end = omp_get_wtime();
    std::cout << "T" << 1 <<" = " << end - start << std::endl;
    T1 = end - start;
        for (int i = 1; i < 8; i++) {
        omp_set_num_threads(threads[i]);
        start = omp_get_wtime();
        x = jacobi_method_schedule(A, b, n, max_iter, tol, "dynamic");
        end = omp_get_wtime();
        std::cout << "T" << threads[i] <<" = " << end - start << ", S" << threads[i] << " = " << T1/(end - start) << std::endl;
    }

    std::cout << "guided" <<std::endl;
    //guided
    omp_set_num_threads(1);
    start = omp_get_wtime();
    x = jacobi_method_schedule(A, b, n, max_iter, tol, "guided");
    end = omp_get_wtime();
    std::cout << "T" << 1 <<" = " << end - start << std::endl;
    T1 = end - start;
        for (int i = 1; i < 8; i++) {
        omp_set_num_threads(threads[i]);
        start = omp_get_wtime();
        x = jacobi_method_schedule(A, b, n, max_iter, tol, "guided");
        end = omp_get_wtime();
        std::cout << "T" << threads[i] <<" = " << end - start << ", S" << threads[i] << " = " << T1/(end - start) << std::endl;
    }



    return 0;
}
