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

//������� �������� ��� ���������� ���� long long
long long min(long long a, long long b)
{
	return a<b?a:b;
}

/*
 * ����� Position �������� � ���� ���������� ������� 
 * ������ �� ����� � ������ ������ � ���
 */
class Position
{
	vector<int> mas;	//i-�� ������� ������� - ����� ��������� �����, ������� ����� �� i-�� �����������

	//��������, ���� �� ����� �� i-�� ����������� ����� �� j-��
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
	//�������� ������� �� � ������
	Position(long long n)
	{
		//����� ����������� ���� ����� �������, ��� ����� � ������� ��������� � ���������� n.
		//����� ��� ���� �������� ����� n(�������� ������������) � ������� ��������� � ���������� n(���������� ������).
		mas.resize(::n);
		for(int i = 0; i<mas.size(); i++)
		{
			mas[i] = n%::n;
			n /= ::n;
		}
	}
	//������� � �������, ����� ������� �� ������� ������
	void next_permutation()
	{
		// ������ ��������� ������������ ���������� 1 � ����� � ������� ��������� � ��������� n
		mas[0]++;
		for(int i = 0; i<mas.size()-1; i++)
			if(mas[i]<n) break;
			else
			{				
				mas[i+1] += mas[i] / n;
				mas[i] %= n;
			}		
	}
	//��������, ������������� �� ������� ����������� ������
	bool Check()
	{		
		for(int i = 0; i<mas.size(); i++)
			for(int j = i+1; j<mas.size(); j++)
				if(Hit(i, j)) return false;
		return true;
	}
	//����� ������� �� �����
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

//������� ���������� � �������. �������� �� O(Log(y))
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
    int myid,		//id ��������
		numprocs;	//���������� ���������
	MPI_Status status;

	//�������������� MPI
    if(MPI_Init(&argc,&argv)) return -1;
	int rcode;

	//����������� id �������� � �� ����������.
    rcode = MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	FAIL_CHECK;
    rcode = MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
	FAIL_CHECK;

	//������� � id = 0 ��������� n - ���������� ������ � ������ �����.
	if(!myid) cin>>n;

	//���������� ������� ������ ���� ���������
	rcode = MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	FAIL_CHECK;

	/*
	 * ���� ��������� ����� ������:
	 * �������� ��� ��������� �������� ����������� ������
	 * � ���������, ����� ���������� �� ��� �������������
	 * ����������� ������.
	 *
	 * �������� ���������� ��������� �����������: ����� ����������
	 * �� ��� ��������� ������� ������, � �����, ��� � ������
	 * ������ (��� ��������, ������ ��� ����������) ���������
	 * ������ ���� �����. ��������, ��� ���� � ������ ���������
	 * ����� ������ �����, �� ������ ������� �� �������������
	 * ����������� ������: ����� �� ����� ������ ���� ����
	 * �����. ����� �����, ��� ��� ���������� ������ ���������
	 * � ����������� �����, � ������ ������ ����� ����������
	 * ����� ���� �����.
	 *
	 * ��� ����� ������� ����� ����� ������������ ������������
	 * ���������. ��������� ����� ���������� �������, �������
	 * �� ������ ���������, ����� ���� �������� ��� ����������
	 * �� ����� ��������� � �������� �� ������ ��������
	 * �������� ����� ����� ������ ��������� �������. �����
	 * ����� ������ ����������, ���������� � ������ ��������.
	 */

	//���������� ����� ���������� �������
	long long position_count = power(n, n);
	
	/*
	 * ������������ ��� ������� ��������� �������:
	 * ����� ����� �� 0-��� ����������� ����� �� ��������� a0,
	 * ����� �� 1-�� ����������� ����� �� ��������� a1, �����
	 * �� n-�� ����������� ����� �� ��������� an.
	 * ����� ����� ������� Pn = a0 + a1*n + ... + an*n^(n-1)
	 * a in [0, n)
	 */

	//��������� ����� �������, � �������� ������ ������� ���������� �������
	//� ���������� ��������, ������� �� ����� ����������.
	long long need_count = position_count / numprocs;	//��������� �������
	long long begin = need_count*myid;					//����� �������, � �������� �������� �������
	begin+=min(position_count % numprocs, myid);
	if(position_count % numprocs > myid)
		need_count++;
	
	long long counter = 0;	//�������, ������� ����� ��������� ���������� "�������" �������
	Position p(begin);

	//���������� ��� �������, ������� ������ ��������� � ���������� ��������
	for(int i = 0; i<need_count; i++, p.next_permutation())
		if(p.Check())
		//���� ������� ������������� ������� ������
		{
			//����������� �������
			counter++;
			//������� ������� �� �����
			p.Print();
		};

	//��������� ���������� "�������" �������, ������� �� ��������� � ������ ��������
	long long buf;
	MPI_Reduce(&counter, &buf, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	FAIL_CHECK;
	counter = buf;
	
	//������� ������� ������ �� �����
	if(!myid)
		cout<<"Number of solutions: "<<counter<<endl;	

	//���������������� MPI
	rcode = MPI_Finalize();	
    return rcode;
}