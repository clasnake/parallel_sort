#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>
#define random(x) (rand()%x)
using namespace std;

long *data_generation(long n)
{
	long *array=new long[n];
	srand((long)time(0));
	for(long i=0;i<n;i++)
	{
		array[i]=random(2147483647);
	}
	return array;
}

int main(int argc, char **argv)
{
	if(argc<3)
	{
		cout<<"please check your input"<<endl;
		exit(0);
	}
	long n=atol(argv[1]);
	cout<<" "<<n<<" "<<argv[2]<<endl;
	fstream fs;
	fs.open(argv[2],ios_base::out);
	srand((long)time(0));
	fs<<argv[1]<<endl;
	for(long i=0;i<n-1;i++)
	{
		fs<<random(2147483647)<<",";
	}
	fs<<random(2147483647);
	fs.close();
	cout<<endl;
	return 0;

	
}
