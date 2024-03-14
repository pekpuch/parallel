#include <iostream>
#include <omp.h>
#include <vector>
#include <cmath>

const double a = -4.0; 
const double b = 4.0; 
const int nsteps = 40000000; 

double func(double x) { 
    return exp(-x * x); 
} 

double integrate_omp(double (*func)(double), double a, double b, int n, int thread_numb) { 
    double h = (b - a) / n;  
    double sum = 0.0; 
    #pragma omp parallel num_threads(thread_numb)  
    {    
        double sumloc = 0.0;
        #pragma omp for 
        for (int i = 0; i < n; i++) 
            sumloc += func(a + h * (i + 0.5));
        #pragma omp atomic 
        sum += sumloc;
    }
    sum *= h; 
    return sum;
}

double run_parallel (double a, double b, int nsteps, int thread_numb) {
    double t = omp_get_wtime();
    double res = integrate_omp(func, a, b, nsteps, thread_numb);
    t = omp_get_wtime() - t;
    return t;
}

int main() {
    int threads[8] = {1,2,4,7,8,16,20,40};
    double T1 = run_parallel(a, b, nsteps, 1);
    std::cout << "T1 = " << T1 << std::endl;
    for(int i = 1; i < 8; i++) {
            double TN = run_parallel(a, b, nsteps,  threads[i]);
            double S = T1/TN;
            std::cout << "T" << threads[i] << " = " << TN << " S" << threads[i] << " = " << S << std::endl;
        }
    return 0; 
}