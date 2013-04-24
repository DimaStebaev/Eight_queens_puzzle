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

long long C(int n, int k)
{
	if(k>n) throw 1;

	static vector< vector<long long> > c;	

	for(int i = c.size(); i<=n;i++)
	{
		c.push_back(vector<long long>(i+1));
		c[i][0] = 1;
		for(int j = 1; j<=i; j++)
		{
			c[i][j] = c[i-1][j-1];
			if(j<i) c[i][j] += c[i-1][j];
		}
	}

	return c[n][k];
}

long long min(long long a, long long b)
{
	return a<b?a:b;
}

class Position
{
	vector<int> mas;

	bool Hit(int a, int b)
	{
		int n = mas.size();
		int ax = a / n, ay = a % n;
		int bx = b / n, by = b % n;

		if(ax == bx) return true;
		if(ay == by) return true;

		if(ax - ay == bx - by) return true;
		if(ax + ay == bx + by) return true;

		return false;
	}
public:
	Position(long long n)
	{
		int p = 0;
		int s = ::n*::n;
		for(int i = 0; i<::n;p++)
		{
			long long count = C(s - p - 1, ::n - i - 1);
			if(n < count)
			{
				mas.push_back(p);
				i++;				
			}
			else			
			{
				n-= count;
			}
		}
	}
	void next_permutation()
	{
		int len = mas.size()*mas.size();
		if(mas.back() < len - 1)
		{
			mas.back()++;
			return;
		}

		for(int i = mas.size()-2; i>=0; i--)
		{
			if(mas[i+1] - mas[i] > 1)
			{
				mas[i]++;
				for(int j = i+1; j<mas.size(); j++)
					mas[j] = mas[j-1]+1;
				return;
			}
		}
	}
	bool Check()
	{		
		for(int i = 0; i<mas.size(); i++)
			for(int j = i+1; j<mas.size(); j++)
				if(Hit(mas[i], mas[j])) return false;
		return true;
	}
	void Print()
	{
		//cout<<"Solution:"<<endl;
		cout<<'+';
		for(int i = 0; i<n; i++) cout<<'-';
		cout<<'+'<<endl;

		int next = 0;
		for(int i=0; i<n; i++)
		{			
			cout<<'|';
			for(int j = 0; j<n; j++)
			{
				if(next < n && i*n+j == mas[next])
				{
					cout<<'*';
					next++;
				}
				else cout<<".";
			}
			
			cout<<'|'<<endl;
		}
		cout<<'+';
		for(int i = 0; i<n; i++) cout<<'-';
		cout<<'+'<<endl;
	}
};

int main(int argc,char *argv[])
{
    int myid, numprocs;    
	MPI_Status status;

    if(MPI_Init(&argc,&argv)) return -1;
	int rcode;
    rcode = MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	FAIL_CHECK;
    rcode = MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
	FAIL_CHECK;
    
    fprintf(stdout,"Process %d of %d\n",
	    myid, numprocs);
    fflush(stdout);

	if(!myid)
		cin>>n;

	rcode = MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	FAIL_CHECK;

	long long position_count = C(n*n, n);

	//cout<<"All - "<<position_count<<endl;
	long long need_count = position_count / numprocs;
	long long begin = need_count*myid;	
	begin+=min(position_count % numprocs, myid);
	if(position_count % numprocs > myid)
		need_count++;

	//cout<<myid<<": ["<<begin<<", "<<begin+need_count<<")"<<endl;

	long long counter = 0;
	Position p(begin);	
	for(int i = 0; i<need_count; i++, p.next_permutation())
		if(p.Check())
		{
			counter++;
			p.Print();
		};

	cout<<"Number of solutions: "<<counter<<endl;

	rcode = MPI_Finalize();	
    return rcode;
}