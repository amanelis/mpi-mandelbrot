//We are using the excecutable 'load4' with N = 200 and datatype as double

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "mpi.h"
#define N 200

int main(int argc, char** argv)
{
   	int i, j, k;
   	
	double *A[N];
	double *B[N]; 
	double *C[N]; 
	
	double totalTime;
        double totalTime1, totalTime2;

	for(i = 0; i < N; i++) 
	{
	    
	    A[i] = (double *) calloc (N, sizeof(float));
	    B[i] = (double *) calloc (N, sizeof(float));
	    C[i] = (double *) calloc (N, sizeof(float));
		
		for( j = 0; j < N; j++)
		{
			A[i][j] = (float)(i+j);
			B[i][j] = (float)(i-j);			
		}	   
	}
	
	totalTime1 = MPI_Wtime();
	
	while(1)
	{
      		for (i =0 ; i< N; i++)
		{
         		for (j =0 ; j< N ; j++)	
             			{
             				C[i][j] = 0;
	             			for (k=0; k < N; k++)
                   				C[i][j] = C[i][j] + A[i][k]*B[k][j];
                   		}
		
	
			totalTime2 = MPI_Wtime();
        		totalTime = totalTime2 - totalTime1;
		}

		//cout << totalTime << endl;
                //if(totalTime > 900)break;
 
       }       
  
	//	cout << "Total time is "<< totalTime << endl;

 
}
