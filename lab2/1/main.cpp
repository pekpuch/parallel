#include <iostream>
#include <omp.h>
#include <vector>


double run_parallel(int n, int thread_numb, std::vector<std::vector<double>>& a, std::vector<double>& b) {  
    double t = omp_get_wtime();  
    #pragma omp parallel num_threads(thread_numb) 
    {
        #pragma omp for 
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
                a[i][j] = i + j;
            b[i] = i;
        }
    }
    #pragma omp parallel num_threads(thread_numb) 
    {
        #pragma omp for schedule(guided)
        for (int i = 0; i < n; i++) 
            for (int j = 0; j < n; j++) 
                a[i][j] *= b[j];
    }
    t = omp_get_wtime() - t;
    
    return t;
}

int main (int argc, char** argv) {
    int sizes[2] = {20000, 40000};
    int n;
    int numbers[8] = {1, 2,4,7,8,16,20,40};

    for(int j = 0; j < 2; j++) {
        n = sizes[j];
        std::vector<std::vector<double>> a(n, std::vector<double>(n));
        std::vector<double> b(n);
        if (j == 0) std::cout << "For 20000 size: " << std::endl;
        else std::cout << "For 40000 size: " << std::endl;
        double T1 = run_parallel(n, 1, a, b);
        std::cout << "T1 = " << T1 << std::endl;
        for(int i = 1; i < 8; i++) {
            double TN = run_parallel(n, numbers[i], a, b);
            double S = T1/TN;
            std::cout << "T" << numbers[i] << " = " << TN << " S" << numbers[i] << " = " << S << std::endl;
        }
    }
}