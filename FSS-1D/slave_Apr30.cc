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

	finalReport.computedColumns = 0;
	finalReport.noOfRequests = 0;
	finalReport.totalTime = 0;
	finalReport.commTime = 0;
	finalReport.computeTime = 0;
	finalReport.slaveId = myId;
	int incLoad=0;

	
		
	initLoad[0] = myId;
	initLoad[1] = loads[myId-1]; //loads is defined in power.h
	
	//comTimeStart=MPI_Wtime();	
	MPI_Send(initLoad, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	//comTimeStop=MPI_Wtime(); //Communication Time
	//finalReport.commTime+=comTimeStop-comTimeStart;	

      /*        
        if(distribution == 1)
        {
          //   outFile << "Robin factor is " << robinFactor << endl;
             for(int i = 0; i < DOTSX; i++)
                columns[i]=(i % robinFactor) * DOTSX/robinFactor+i/robinFactor;

        }
        else{
                for(int i = 0; i < DOTSX; i++)
                        columns[i] = i;
        } 
       */

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
	
		Chunk* chunk = new Chunk();
		
		MPI_Recv(work, 4, MPI_INT, 0, 4, MPI_COMM_WORLD, &status);
		
		comTimeStop=MPI_Wtime();
		
		finalReport.commTime += comTimeStop - comTimeStart - (float(work[3])/1000);
		
			
		chunk->startColumn = work[0];
		chunk->noOfColumns = work[1];


		Chunk* newChunk=chunk;	
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
			
			float cx, cy;
			float a, oldA, bI;
			float as, bis;
			float lengthZ;

			const float MinX = -2;
               		const float MaxX = 2;
	         	const float MinY = -2;
	                const float MaxY = 2;

              		float dx = (MaxX - MinX)/DOTSX;
	                float dy = (MaxY - MinY)/DOTSY;

		
			for(int i = cs; i < cs1;  i++)
			{
				cx = MinX + i*dx;
          
                        //        cx = MinX + columns[i]*dx;
			
				for(int j = 0; j < DOTSY; j++)
				{
					
					cy = MinY + j*dy;
					
					a = 0; bI = 0;
					as = 0; bis = 0;
					lengthZ =0;
					
					for(k=0; k< MaxIteration; k++)
					{
					   if(lengthZ > 4)
						break;

					        as = a*a;
					        bis = bI*bI;
					        
						oldA = a;
						a = as - bis + cx;
						bI = 2*oldA*bI + cy;
						lengthZ = as + bis ;
					}
					
					putData[counter++]= k;
				
				}
				
			}
				
								
			compTimeStop=MPI_Wtime();  //Compute time
			finalReport.computeTime += compTimeStop-compTimeStart;
			
			myChunk.startColumn = newChunk->startColumn;
			myChunk.noOfColumns = newChunk->noOfColumns;
			finalReport.computedColumns += myChunk.noOfColumns;
			
			
			putData[0] = finalReport.slaveId;
			putData[1] = currentLoad;
			putData[2] = 0;
			putData[3] = cs;
			putData[4] = cs1;
			//if returning results then use 'counter' inside send after putData, otherwise use 3	
			MPI_Send(putData, 3, MPI_FLOAT, 0, 3, MPI_COMM_WORLD);
			
			comTimeStart=MPI_Wtime();
						
			//counter = 9;
		
		}
 
 	}
	
	
};

	
