#include <iostream>
#include <omp.h>
#include <vector>
#include <cmath>

// Функция вывода элементов матрицы
void Output(double ** mas, int rows, int cols) {
	for(int i = 0; i < rows; i++) {
		for(int j = 0; j < cols; j++) std::cout << mas[i][j] << " ";
        std::cout <<std::endl;
	}
    
}
// Транспонирование матрицы
double ** Transpone1(double ** mas, int rows, int cols) {
	double ** rez;
	rez = (double ** ) malloc(cols * sizeof(double * ));
	for(int i = 0; i < cols; i++) {
		rez[i] = (double * ) malloc(rows * sizeof(double));
		for(int j = 0; j < rows; j++) rez[i][j] = mas[j][i];
	}
	return rez;
}

// Получение матрицы без i-й строки и j-го столбца
// (функция нужна для вычисления определителя и миноров)
double ** GetMatr(double ** mas, int rows, int cols, int row, int col) {
	int di, dj;
	double ** p = (double ** ) malloc((rows - 1) * sizeof(double * ));
	di = 0;
	for(int i = 0; i < rows - 1; i++) { // проверка индекса строки
		if(i == row) // строка совпала с вычеркиваемой
			di = 1; // - дальше индексы на 1 больше
		dj = 0;
		p[i] = (double * ) malloc((cols - 1) * sizeof(double));
		for(int j = 0; j < cols - 1; j++) { // проверка индекса столбца
			if(j == col) // столбец совпал с вычеркиваемым
				dj = 1; // - дальше индексы на 1 больше
			p[i][j] = mas[i + di][j + dj];
		}
	}
	return p;
}
// Рекурсивное вычисление определителя
double Determinant(double ** mas, int m) {
	int k;
	double ** p = 0;
	double d = 0;
	k = 1; //(-1) в степени i
	if(m == 1) {
		d = mas[0][0];
		return (d);
	}
	if(m == 2) {
		d = mas[0][0] * mas[1][1] - (mas[1][0] * mas[0][1]);
		return (d);
	}
	if(m > 2) {
		for(int i = 0; i < m; i++) {
			p = GetMatr(mas, m, m, i, 0);
			d = d + k * mas[i][0] * Determinant(p, m - 1);
			k = -k;
		}
	}
	return (d);
}
// Обратная матрица
double ** Mreverse1(double ** mas, int m) {
	double ** rez = (double ** ) malloc(m * sizeof(double * ));
	double det = Determinant(mas, m); // находим определитель исходной матрицы
	#pragma omp parallel for
	for(int i = 0; i < m; i++) {
		rez[i] = (double * ) malloc(m * sizeof(double));
		for(int j = 0; j < m; j++) {
			rez[i][j] = Determinant(GetMatr(mas, m, m, i, j), m - 1);
			if((i + j) % 2 == 1) // если сумма индексов строки и столбца нечетная
				rez[i][j] = -rez[i][j]; // меняем знак минора
			rez[i][j] = rez[i][j] / det;
		}
	}

	#pragma omp parallel for
	for(int i = 0; i < m; i++) {
		for(int j = 0; j < m; j++) rez[i][j] = rez[j][i];
	}

	return rez;
}

double ** Mreverse2(double ** mas, int m) {
	double ** rez = (double ** ) malloc(m * sizeof(double * ));
	#pragma omp parallel 
	{
	double det = Determinant(mas, m); // находим определитель исходной матрицы
	for(int i = 0; i < m; i++) {
		rez[i] = (double * ) malloc(m * sizeof(double));
		for(int j = 0; j < m; j++) {
			rez[i][j] = Determinant(GetMatr(mas, m, m, i, j), m - 1);
			if((i + j) % 2 == 1) // если сумма индексов строки и столбца нечетная
				rez[i][j] = -rez[i][j]; // меняем знак минора
			rez[i][j] = rez[i][j] / det;
		}
	}
		for(int i = 0; i < m; i++) {
			for(int j = 0; j < m; j++) rez[i][j] = rez[j][i];
	}
	}
	return rez;

}


int main() {

	int n = 4;
    double** mas = (double**)malloc(n * sizeof(double*));	
    
    double * b = (double*)malloc(n * sizeof(double));
    double * sol= (double*)malloc(n * sizeof(double));
    for(int i = 0; i< n; i++ ) {
        mas[i] = (double*)malloc(n * sizeof(double));
        b[i] = i + 1;
        sol[i] = 0;
        for(int j = 0; j< n; j++ ) {
            if(i==j) mas[i][j] = 2;
            else mas[i][j] = 1;
        }
    }

	// Находим обратную матрицу
	double t = omp_get_wtime();  
	double ** mas_reverse = Mreverse1(mas, n);
	t = omp_get_wtime() - t;
	std::cout << "T1 = " << t << std::endl;

	t = omp_get_wtime();  
	mas_reverse = Mreverse2(mas, n);
	t = omp_get_wtime() - t;
	std::cout << "T2 = " << t << std::endl;

	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			sol[i] += mas_reverse[i][j] * b[i];
		}
	}

	return 0;
}
