#include <iostream>
#include <omp.h>
#include <vector>
#include <cmath>

double eps = 0.0000000000001;

int main() {
    int n = 4;
    std::vector<std::vector<double>> matrix(n, std::vector<double>(n));
    double solution[n]; 
    double xs[n]; 
    double b[n]; 

    for(int i = 0; i< n; i++ ) {
        for(int j = 0; j< n; j++ ) {
            if(i==j) matrix[i][j] = 2;
            else matrix[i][j] = 1;
        }
        xs[i] = 0;
        b[i] = i+1;
    }
   do {
        for(int i = 0; i< n; i++) {
            solution[i] = b[i];
            for(int j = 0; j< n; j++ ) {
                if (i != j) {
                    solution[i] -= matrix[i][j] * xs[j];
                }
            }
            solution[i] /= matrix[i][i];
        }
        for (int i = 0; i< n; i++) {
            if(fabs(solution[i] - xs[i]) < eps) break;
        }
        for (int i = 0; i< n; i++) {
            xs[i] = solution[i];
        }

    } while (1);

    for (int i = 0; i < n; i++) {
        std::cout << solution[i] << std::endl;

    }


    return 0; 
}