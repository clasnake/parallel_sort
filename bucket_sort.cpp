#include "mpi.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <cstdlib>
#define INF (-1)
using namespace std;

long original_len;//length of the unsorted data
int rank;//rank of the current process
int proc_number;//number of processes

//load the unsorted data
long *data_loading(char *dir)
{
	fstream read;
	string temp;
	long line=0;
	read.open(dir, ios::in);
	getline(read, temp);
	long size=atol(temp.c_str());
	cout<<size<<endl;
	original_len=size;
	long *array=new long[size+1];
	long i=0;

	getline(read, temp);
	char *str_arr=new char[temp.size()+1];
	strcpy(str_arr, temp.c_str());
	char *list=strtok(str_arr, ",");
	while(NULL!=list)
	{
		array[i++]=atol(list);
		list=strtok(NULL, ",");
	}
	read.close();
	return array;
}

//IncOrder for qsort
int IncOrder(const void *e1, const void *e2)
{
	return (*((long *)e1)-*((long *)e2));
}

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		cout<<"Please check your input"<<endl;
		exit(0);
	}
	long proc_data_size;//size of data on the current process
	long *proc_data;//data on the current process
	long *original;//the unsorted data
	long *final;//the sorted data
	long *pivot_list;//the pivot list
	long *proc_buckets;//the buckets in the current process
	long *final_buckets;//the bucket after alltoall function
	double exec_time;

	MPI_Init(&argc, &argv);
	MPI_Barrier(MPI_COMM_WORLD);
	exec_time=-MPI_Wtime();
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_number);

	if(0==rank)
	{	
		original=data_loading(argv[1]);
		proc_data_size=original_len/proc_number;
		cout<<proc_data_size<<endl;

	}
	MPI_Bcast(&original_len, 1, MPI_LONG, 0, MPI_COMM_WORLD);
	MPI_Bcast(&proc_data_size, 1, MPI_LONG, 0, MPI_COMM_WORLD);
	proc_data=new long[proc_data_size];

	final=new long[original_len];
	pivot_list=new long[proc_number];
	MPI_Scatter(original, proc_data_size, MPI_LONG, proc_data, proc_data_size, MPI_LONG, 0, MPI_COMM_WORLD);

	//initial local sort
	qsort(proc_data, proc_data_size, sizeof(long), IncOrder);
	
	MPI_Gather(proc_data, 1, MPI_LONG, pivot_list, 1, MPI_LONG, 0, MPI_COMM_WORLD);
	if(0==rank)
	{
		qsort(pivot_list, proc_number, sizeof(long), IncOrder);
	}

	proc_buckets=new long[original_len];

	//initialize proc_buckets
	for(long i=0;i<original_len;i++)
	{
		proc_buckets[i]=INF;
	}

	long *index=new long[proc_number];

	//initialize index
	for(int i=0;i<proc_number;i++)
	{
		index[i]=0;
	}
	MPI_Bcast(pivot_list, proc_number, MPI_LONG, 0, MPI_COMM_WORLD);


	//update proc_buckets
	for(long i=0;i<proc_data_size;i++)
	{
		for(int j=0;j<proc_number-1;j++)
		{
			if(proc_data[i]>=pivot_list[j]&&proc_data[i]<pivot_list[j+1])
			{
				proc_buckets[j*proc_data_size+index[j]]=proc_data[i];
				index[j]=index[j]+1;
			}
		}
		if(proc_data[i]>=pivot_list[proc_number-1])
		{
			proc_buckets[(proc_number-1)*proc_data_size+index[proc_number-1]]=proc_data[i];
			index[proc_number-1]=index[proc_number-1]+1;
		}
	}

	//creation of a new datatype BUCKETS
	MPI_Datatype BUCKETS;
	MPI_Type_contiguous(proc_data_size, MPI_LONG, &BUCKETS);
	MPI_Type_commit(&BUCKETS);


	final_buckets=new long[proc_number*proc_data_size];
	for(long i=0;i<original_len;i++)
	{
		final_buckets[i]=0;
	}

	//the alltoall function to get the final buckets in processes
	MPI_Alltoall(proc_buckets, 1, BUCKETS, final_buckets, 1, BUCKETS, MPI_COMM_WORLD);
	
	MPI_Type_free(&BUCKETS);

	long *result;
	long count=0;
	for(long i=0;i<original_len;i++)
	{
		if(final_buckets[i]!=INF)
		{
			count++;
		}
	}
	result=new long[count];
	count=0;
	for(long i=0;i<original_len;i++)
	{
		if(final_buckets[i]!=INF)
		{
			result[count++]=final_buckets[i];
		}
	}

	qsort(result, count, sizeof(long), IncOrder);


	//Gather the results to rank 0
	int *recv_cnt=new int[proc_number];
	long *sorted=new long[original_len];
	int *displs=new int[proc_number];

	MPI_Gather(&count, 1, MPI_LONG, recv_cnt, 1, MPI_LONG, 0, MPI_COMM_WORLD);
	displs[0]=0;
	for(int i=1;i<proc_number;i++)
	{
		displs[i]=displs[i-1]+recv_cnt[i-1];
	}

	MPI_Gatherv(result, count, MPI_LONG, sorted, recv_cnt, displs, MPI_LONG, 0, MPI_COMM_WORLD);
	exec_time+=MPI_Wtime();
	cout<<"time of rank "<<rank<<":"<<exec_time<<endl;
	//print the sorted data on rank 0
//	if(0==rank)
//	{
//		for(long i=0;i<original_len;i++)
//		{
//			cout<<sorted[i]<<" ";
//		}
//		cout<<endl;
//	}

	MPI_Finalize();
	return 0;
}
