// chess.cpp : Defines the entry point for the console application.
//

#include "mpi.h"
#include <iostream>
#include <stack>
#include <list>
#include <vector>

using namespace std;

int n;

#define FAIL_CHECK if(rcode) {MPI_Finalize(); return rcode;}

//Функция минимума для переменных типа long long
long long min(long long a, long long b)
{
	return a<b?a:b;
}

/*
 * Класс Position содержит в себе конкретную позицию 
 * ферзей на доске и логику работы с ней
 */
class Position
{
	vector<int> mas;	//i-ый элемент массива - номер вертикали ферзя, который стоит на i-ой горизонтали

	//Проверка, бъёт ли ферзь на i-ой горизонтали ферзя на j-ой
	bool Hit(int i, int j)
	{		
		int ax = i, ay = mas[i];
		int bx = j, by = mas[j];

		if(ax == bx) return true;
		if(ay == by) return true;

		if(ax - ay == bx - by) return true;
		if(ax + ay == bx + by) return true;

		return false;
	}
public:
	//Создание позиции по её номеру
	Position(long long n)
	{
		//Можно представить себе номер позиции, как число в системе счисления с основанием n.
		//Тогда код ниже преводит число n(аргумент конструктора) в систему счисления с основанием n(количество ферзей).
		mas.resize(::n);
		for(int i = 0; i<mas.size(); i++)
		{
			mas[i] = n%::n;
			n /= ::n;
		}
	}
	//Переход к позиции, номер которой на единицу больше
	void next_permutation()
	{
		// Данная процедура эквивалентна добавлению 1 к числу в системе счисления с основание n
		mas[0]++;
		for(int i = 0; i<mas.size()-1; i++)
			if(mas[i]<n) break;
			else
			{				
				mas[i+1] += mas[i] / n;
				mas[i] %= n;
			}		
	}
	//Проверка, удовлетворяет ли позиция ограничению задачи
	bool Check()
	{		
		for(int i = 0; i<mas.size(); i++)
			for(int j = i+1; j<mas.size(); j++)
				if(Hit(i, j)) return false;
		return true;
	}
	//Вывод позиции на экран
	void Print()
	{
		//cout<<"Solution:"<<endl;
		cout<<'+';
		for(int i = 0; i<n; i++) cout<<'-';
		cout<<'+'<<endl;
		
		for(int i=0; i<n; i++)
		{			
			cout<<'|';
			for(int j = 0; j<n; j++)
			{
				if(mas[i] == j)
					cout<<'*';	
				else 
					cout<<".";
			}
			
			cout<<'|'<<endl;
		}
		cout<<'+';
		for(int i = 0; i<n; i++) cout<<'-';
		cout<<'+'<<endl;
	}
};

//Быстрое возведение в степень. Работает за O(Log(y))
long long power(long long x, long long y)
{
	if(!y) return 1;
	long long ans = 1;
	if(y%2)
		return power(x, y-1)*x;
	else
	{
		long long s = power(x, y/2);
		return s*s;
	}
}


int main(int argc,char *argv[])
{
    int myid,		//id процесса
		numprocs;	//количество процессов
	MPI_Status status;

	//Инициализируем MPI
    if(MPI_Init(&argc,&argv)) return -1;
	int rcode;

	//Запрашиваем id процесса и их количество.
    rcode = MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	FAIL_CHECK;
    rcode = MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
	FAIL_CHECK;

	//Процесс с id = 0 считывает n - количество ферзей и размер доски.
	if(!myid) cin>>n;

	//Отправляем входные данные всем процессам
	rcode = MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	FAIL_CHECK;

	/*
	 * Идея алгоритма очень проста:
	 * переберём все возможные варианты расстановки ферзей
	 * и посчитаем, какое количество из них удовлетворяет
	 * ограничению задачи.
	 *
	 * Применим достаточно очевидную оптимизацию: будем перебирать
	 * не все возможные позиции ферзей, а такие, где в каждой
	 * строке (или столбике, смотря как посмотреть) находится
	 * только один ферзь. Очевидно, что если в строке находится
	 * более одного ферзя, то данная позиция не удовлетворяет
	 * ограничению задачи: ферзи на одной строке бьют друг
	 * друга. Кроме этого, так как количество ферзей совпадает
	 * с количеством строк, в каждой строке будет находиться
	 * ровно один ферзь.
	 *
	 * При таком подходе можно легко организовать параллельную
	 * обработку. Посчитаем общее количество позиций, которые
	 * мы должны проверить, после чего разделим это количество
	 * на число процессов и запустим на каждом процессе
	 * проверку своей части общего множества позиций. После
	 * этого сложим результаты, полученные в каждом процессе.
	 */

	//Рассчитаем общее количество позиций
	long long position_count = power(n, n);
	
	/*
	 * Перенумеруем все позиции следующим образом:
	 * Пусть ферзь на 0-вой горизонтали стоит на вертикале a0,
	 * ферзь на 1-ой горизонтали стоит на вертикале a1, ферзь
	 * на n-ой горизонтали стоит на вертикале an.
	 * Тогда номер позиции Pn = a0 + a1*n + ... + an*n^(n-1)
	 * a in [0, n)
	 */

	//Определим номер позиции, с которого начнёт перебор конкретный процесс
	//и количество поззиций, которое он будет перебирать.
	long long need_count = position_count / numprocs;	//Количетво позиций
	long long begin = need_count*myid;					//Номер позиции, с которого начнётся перебор
	begin+=min(position_count % numprocs, myid);
	if(position_count % numprocs > myid)
		need_count++;
	
	long long counter = 0;	//Счётчик, который будет содержать количество "хороших" позиций
	Position p(begin);

	//Перебираем все позиции, которые должны перебрать в конкретном процессе
	for(int i = 0; i<need_count; i++, p.next_permutation())
		if(p.Check())
		//Если позиция удовлетворяет условию задачи
		{
			//Увеличиваем счётчик
			counter++;
			//Выводим позицию на экран
			p.Print();
		};

	//Суммируем количество "хороших" позиций, которых мы насчитали в каждом процессе
	long long buf;
	MPI_Reduce(&counter, &buf, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	FAIL_CHECK;
	counter = buf;
	
	//Выводим решение задачи на экран
	if(!myid)
		cout<<"Number of solutions: "<<counter<<endl;	

	//Деинициализируем MPI
	rcode = MPI_Finalize();	
    return rcode;
}