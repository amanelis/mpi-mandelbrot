//FSS-1D

#include <fstream.h>
#include <iostream.h>
#include "schedule.h"
#include "schedule_aux.h"
#include "mpi.h"
#include "math.h"
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <unistd.h>
#include "input.h"
#include "power.h"


Slave::Slave(int myId)
{
	
	
	int initLoad[2];
	MPI_Status status;
	MPI_Request* request;
	int currentLoad;
	Chunk myChunk;
	
	int dimParameter = 2*(DOTSX * DOTSY)/(totPower);	
	float *putData;
//	putData = (float*) calloc (20, sizeof(float));
//	putData = (float*) calloc (dimParameter+10, sizeof(float));
	
	putData = new float[dimParameter+10];

	memset(putData,0,(dimParameter+10)*sizeof(float));

	int work[4];
	int cs,cs1,cn;
	int k;
	double totTimeStart,comTimeStart,totTimeStop,comTimeStop,compTimeStart,compTimeStop;
	double cx, cy;
        double a, bI;
        double as, bis;


	finalReport.computedColumns = 0;
	finalReport.noOfRequests = 0;
	finalReport.totalTime = 0;
	finalReport.commTime = 0;
	finalReport.computeTime = 0;
	finalReport.slaveId = myId;
	int incLoad=0;

	
		
	initLoad[0] = myId;
	initLoad[1] = loads[myId-1]; //loads is defined in power.h
	
	MPI_Send(initLoad, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	//	startWorking
	
	if(useLoad)
		currentLoad = loads[myId-1];//loads is defined in power.h
	else
		currentLoad = 1;
		
	myChunk.startColumn = 0;
	myChunk.noOfColumns = 0;
	ChunkResult chunkResult = new long[0];
	
  	int noEntry=0;
        int counter=9;

	//MOVED OUTSIDE OF WHILE LOOP	
	putData[0] = finalReport.slaveId;
	putData[1] = currentLoad;
	putData[2] = 0;

	
	totTimeStart=MPI_Wtime();
		
		
		
	MPI_Send(putData, 3, MPI_FLOAT, 0, 3, MPI_COMM_WORLD);
	
	comTimeStart=MPI_Wtime();
	
	
	while(1)
	{	
	
		Chunk* newChunk = new Chunk();
		
		MPI_Recv(work, 4, MPI_INT, 0, 4, MPI_COMM_WORLD, &status);
		
		comTimeStop=MPI_Wtime();
		
		finalReport.commTime += comTimeStop - comTimeStart - (float(work[3])/1000);
		
			
		newChunk->startColumn = work[0];
		newChunk->noOfColumns = work[1];

		finalReport.noOfRequests++;

		if(newChunk->noOfColumns == 0)//
		{
			totTimeStop=MPI_Wtime();
			finalReport.totalTime += totTimeStop-totTimeStart;
			
			delete(chunkResult);
			delete(newChunk);
			
			putData[0] = finalReport.slaveId;
			putData[1] = 0;
			putData[2] = 1;
			putData[3] = finalReport.computedColumns;
			putData[4] = finalReport.noOfRequests;
			putData[5] = finalReport.totalTime;
			putData[6] = finalReport.commTime;
			putData[7] = finalReport.computeTime;
		        putData[8] = noEntry;
		        
		     	MPI_Send(putData, 9, MPI_FLOAT, 0, 3, MPI_COMM_WORLD);
			
			//MPI_Send(putData1, counter, MPI_INT, 0, 33, MPI_COMM_WORLD);
	       		
	       		
	       		free(putData);
		        MPI_Finalize(); 
			exit(0);
		}
		else 
		{
			
			counter = 9;
			cs=newChunk->startColumn;
			cs1=newChunk->startColumn + newChunk->noOfColumns;
			cn=newChunk->noOfColumns;
			noEntry++;
                       
                        compTimeStart = 0; compTimeStop = 0; 

			compTimeStart=MPI_Wtime(); //Computing time
				
			//mandelbrot computation
		
			const double MinX = -2;
                        const double MaxX = 2;
                        const double MinY = -2;
                        const double MaxY = 2;

                        double dx = (MaxX - MinX)/DOTSX;
                        double dy = (MaxY - MinY)/DOTSY;


                        for(int i = cs; i < cs1;  i++)
                        {
                                cx = MinX + i*dx;

                                for(int j = 0; j < DOTSY; j++)
                                {
                                        cy = MinY + j*dy;

                                        a = cx; bI = cy;

                                        for(k = 0; k < MaxIteration; k++)
                                        {
                                            as = a*a;
                                            bis = bI*bI;

                                            if((as+bis) > 4)
                                                break;

                                                bI = 2*a*bI + cy;
                                                a = as - bis + cx;

                                        }//for(k =0....)

                                        putData[counter++]= k;

                                }//for(j=0...)

                        }//for(i=0.....)
	

			compTimeStop=MPI_Wtime();  //Compute time
			finalReport.computeTime += compTimeStop-compTimeStart;
			
			myChunk.startColumn = newChunk->startColumn;
			myChunk.noOfColumns = newChunk->noOfColumns;
			finalReport.computedColumns += myChunk.noOfColumns;
			
			
			putData[0] = finalReport.slaveId;
			putData[1] = currentLoad;
			putData[2] = 0;


			//if returning results then use 'counter' inside send after putData, otherwise use 3	
			MPI_Send(putData, 3, MPI_FLOAT, 0, 3, MPI_COMM_WORLD);
			
			comTimeStart=MPI_Wtime();
						
		}
 
 	}
	
	
};

	
