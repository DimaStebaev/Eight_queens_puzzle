// chess.cpp : Defines the entry point for the console application.
//

#include "mpi.h"
#include <iostream>
#include <stack>
#include <list>
#include <vector>

using namespace std;

int *buffer, buffer_size;
int n;

int magic;

bool debug = 0;

#define DEBUG if(debug)printf

class Task
{
	vector<pair<int, int> > figures;
	vector<pair<int, int> > free;
	int pointer;

	pair<int, int> CalcCoord(int x)
	{
		return make_pair(x / n, x%n);
	}
	int CalcIndex(const pair<int, int> &c)
	{
		return c.first * n + c.second;
	}
	void Print()
	{
		cout<<"Solution:"<<endl;
		for(int i=0; i<n; i++)
		{			
			for(int j = 0; j<n; j++)
			{
				bool found = false;
				for(int k = 0; k<figures.size(); k++)
					if(figures[k] == make_pair(i, j))
					{
						cout<<'*';
						found = true;
						break;
					};
				if(!found) cout<<'.';
			}
			
			cout<<endl;
		}
	}

	bool Hit(const pair<int, int> &a, const pair<int, int> &b)
	{	
		if(a.first == b.first) return true;
		if(a.second == b.second) return true;

		for(int i=0; i<n; i++)
		{
			if( b.first == i && b.second == a.second - a.first + i){
				//DEBUG("Diagonal \\\ i = %d n", i);
				return true;
			}
			if( b.second == i && b.first == a.second + a.first - i) 
			{
				//DEBUG("Diagonal / i = %d\n", i);
				return true;
			}
		}

		//DEBUG("(%d, %d) doesn't hit (%d, %d)\n", a.first, a.second, b.first, b.second);
		return false;
	}
public:
	Task(const vector<pair<int, int> > &figures, const vector<pair<int, int> > &free)
	{
		this->figures.clear();
		this->free.clear();
		this->figures = figures;
		this->free = free;
		pointer = 0;

		//DEBUG("!!!!!!!!!!!!!!!!!! %s \n", Hit(make_pair(0, 1), make_pair(3, 2))?"Hit":"Doesn't hit");
	}
	Task(int *message, int len)
	{
		pointer = message[0];
		bool figure_list = true;
		for(int i = 1; i<len; i++)
			if(message[i] == -1)
				figure_list = false;
			else
				(figure_list?figures:free).push_back(CalcCoord(message[i]));				
	}
	Task SubTask() throw(int)
	{		
		DEBUG("make from free(%d), figures(%d)\n", free.size(), figures.size());		
		
		if(!free.size() && figures.size() == n)
		{
			DEBUG("Solution found\n");
			for(int i=0; i<figures.size(); i++)
				DEBUG("(%d, %d)\n", figures[i].first, figures[i].second);
			Print();
			throw 0;
		}

		if(pointer >= free.size())
		{			

			DEBUG("Can't make subtask (%d, %d)\n", pointer, free.size());
			throw 1;
		}

		vector<pair<int, int> > new_figures = figures;
		vector<pair<int, int> > new_free = free;

		new_figures.push_back(free[pointer]);
		for(int i=0; i<new_free.size(); i++)
			if(Hit(free[pointer], new_free[i]))
			{				
				new_free[i] = new_free[new_free.size()-1];
				new_free.pop_back();
				i--;
			};
		pointer++;
		
		//DEBUG("return: free(%d), figures(%d)\n", new_free.size(), new_figures.size());
		return Task(new_figures, new_free);
	}

	int WriteToBuffer(int *buffer)
	{
		int len = 0;
		buffer[len++] = pointer;		

		for(int i=0; i<figures.size(); i++)
		{			
			buffer[len++] = CalcIndex(figures[i]);
		}

		buffer[len++] = -1;

		for(int i = 0; i<free.size(); i++)
			buffer[len++] = CalcIndex(free[i]);

		return len;
	}
};

int main(int argc,char *argv[])
{
    int myid, numprocs;    
	MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);    
    
    fprintf(stdout,"Process %d of %d\n",
	    myid, numprocs);
    fflush(stdout);

	if(myid!=2) debug = false;

	if(myid)
	{
		//slave
		
		MPI_Recv(&n, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		buffer_size = n*n+n+3;
		buffer = new int[buffer_size];

		MPI_Recv(buffer, buffer_size, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	

		int count;
		MPI_Get_count(&status, MPI_INT, &count);
		if(count == 1 && buffer[0] == -1)
		{
			cout<<"Slave "<<myid<<": nothing to do"<<endl;
		}
		else
		{
			cout<<"Slave "<<myid<<" begins working"<<endl;
			stack<Task> s;
			s.push(Task(buffer, count));
			while(s.size())
			{
				Task task = s.top(); s.pop();
				while(true)
				{
					try
					{
						s.push(task.SubTask());
					}
					catch(int)
					{
						break;
					}
				}
			}
			cout<<"Slave "<<myid<<" is ready"<<endl;
		}		
		MPI_Send(buffer, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

	}else{
		//master
		cin>>n;	
		
		for(int i=1; i<numprocs; i++)		
			MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

		buffer_size = n*n+n+1;
		buffer = new int[buffer_size];

		vector<pair<int, int> > free;
		for(int i=0; i<n; i++)
			for(int j = 0; j<n; j++)
				free.push_back(make_pair(i, j));

		printf("free(%d)\n", free.size());

		cout<<"Master creates tasks"<<endl;
		list<Task> q;
		q.push_back(Task(vector<pair<int, int> >(), free));		
		while(q.size()<numprocs - 1)
		{			
			try
			{
				Task task = q.front(); q.pop_front();
				q.push_back(task.SubTask());
				DEBUG("Maked subtask\n");
				q.push_front(task);
			}
			catch(int)
			{
				if(!q.size()) break;				
			}
		}		

		cout<<"Master sends tasks ("<<q.size()<<")"<<endl;
		for(int i=1; i<numprocs; i++)
		{
			if(!q.size())
			{
				int end = -1;
				cout<<"Master sends empty message to "<<i<<endl;
				MPI_Send(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD);	
				continue;
			}
			Task send_task = q.front(); q.pop_front();
			int len = send_task.WriteToBuffer(buffer);
			cout<<"Master sends task to "<<i<<endl;

			
			for(int j = 0; j<len; j++)
				cout<<" "<<buffer[j];
			cout<<endl;
						

			MPI_Send(buffer, len, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		cout<<"Master waight for slaves"<<endl;
		for(int i=1; i<numprocs; i++)
			MPI_Recv(buffer, buffer_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		cout<<"Ready"<<endl;
	}
    
	delete buffer;
    MPI_Finalize();
    return 0;
}