//FSS-1D

#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include "schedule.h"
#include "mpi.h"
#include "input.h"
#include "power.h"


Master::Master()  :outFile("output.file") //opening output file
{

	
	correction=0;
	changedLoad=0;
	totalVirtualPower=0;
	cout << "Master: configure master..." << endl;
	
	outFile << "Number of slaves: " << noSlaves << endl;
	
	
	//DIMENSION IS CHANGED TO (noSlaves+1) FROM (noSlaves) 
	currentLoads = new int[noSlaves+1]; //master keep storage of load of slaves
	virtualPowers = new float[noSlaves+1];
	availablePowers = new float[noSlaves+1];
	powers = new float[noSlaves+1];
	slaveEnd = new int[noSlaves+1];
	remPwr=new float[noSlaves+1];
	int i;
	remPwr[0]=0;

	//computing virtual/available power for slaves
	for(i = 1; i<= noSlaves; i++){
		currentLoads[i]=1;
		
		virtualPowers[i]=1;
		availablePowers[i]=1;
		slaveEnd[i] = 0;
		powers[i] = 1;
		remPwr[i]=0;
	}
	
	//distribution =1 we use round robin to re-order the loop iterations
       /* 	
	if(distribution == 1)
	{
		outFile << "Robin factor is " << robinFactor << endl;
		for(int i = 0; i < DOTSX; i++)
			columns[i]=(i % robinFactor) * DOTSX/robinFactor+i/robinFactor;

	}
	else
		for(int i = 0; i < DOTSX; i++)
			columns[i] = i;	
		
       */ 
	switch(useLoad)
	{
	    case(0):outFile << "Not using load\n";  //simple scheduling
		break;
	    case(1):outFile << "Using load\n";	    //distributed scheduling
		break;
   	    default:cerr << "Wrong load usage\n";
		exit(1);
	}

	outFile << "Threshold is  " << threshold << endl;
	totalTime = 0;
	computedColumns = 0;
	assignedColumns = 0;
	
	ngoal=0;
}


Master::~Master()
{
	delete virtualPowers;
	delete powers;
	delete slaveEnd;
	delete currentLoads;
	delete availablePowers;
	delete remPwr;
}

void Master::saveResult()
{
	std::ofstream resultFile("result.file");
	if(!resultFile)
	{
		cerr << "Cannot open result file\n";
		exit(1);
	}

	for(int j = 0; j < DOTSX; j++)
		for(int i = 0; i < DOTSY; i++)
			resultFile << resultMatrix[i][j] << " ";

	resultFile.close();
}

/* This function calculate the chunk to be asssigned for each slave */
void Master::workCalculate(int slaveId)
{

	ngoal =  roundsTable[roundNumber[slaveId] * noSlaves + slaveId];

	chunk.startColumn = assignedColumns;
	chunk.noOfColumns = (int)ngoal;

	assignedColumns += chunk.noOfColumns;
	unassignedColumns = DOTSX - assignedColumns;

	roundNumber[slaveId]++;

	currentChunk = (int) ngoal;

	work[0] = chunk.startColumn;
	work[1] = chunk.noOfColumns;
			
	if(nonded==1) // if non dedicated
	     	work[2]=1;
	        else
		work[2]=0;

	if(work[1] > 0)
		chunkCount++;				
}
			
/* This function calculates the chunk size for each round,
   receives the work request from slaves,
   and sends the work to the slaves */ 

void Master::slaveCall()
{
	
	
	int slaveId;
	float slavePower;
	int slaveLoad;
	int position, i;
//	int currentLoads[noSlaves];	//loads of slaves
	int changedLoads[noSlaves+1];
	int endFile = 0, count = 1;
	int initLoad[2];
	totalPower = 0;
	int currentLoad,currentLoad1;
	int startColumn;
	MPI_Status status;
	//Chunk chunk;
	int endCount = 0;
	float goal;
	
	float temp,tempfirstchunk;
	
	maxcomputetime = 0;
	communicationtime = 0;
	maxcommtime = 0;
	
	float *getData;
		
	chunkThreshCount = 0;
        chunkCount = 0;	
	
	int tot=0;
	int longsInMyResult;
	int slowSlaves[noSlaves];
	int sSlaves=0;
	int packageSize;
	char* buffer;
	long chunkResultColumnsNumber;
	ChunkResult chunkResult;
	
	//compute virtual powers for each slave
	
	for(i=1;i<=noSlaves;i++){
		changedLoads[i]=0;
	}

	/* if dtss, then take the powers assigned in the file power.h*/	
	if(useLoad == 1)
	{	
		while(machinePower[count-1]){
			cout<<"Machinepower"<<machinePower[count-1]<<endl;
			powers[count]=machinePower[count-1];
			count++;
		}		 	
	}


	for(int slaveCounter = 0; slaveCounter < noSlaves; slaveCounter++)
	{
   	  //recv initial load from slaves
   		MPI_Recv(initLoad, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
   
        slaveId = initLoad[0];
        slaveLoad = initLoad[1];

        cout << "Master: register slave {" << slaveId << ", " << powers[slaveId] << "}" << endl;
	   
	   virtualPowers[slaveId] = powers[slaveId];
	   totalVirtualPower += powers[slaveId];
	   currentLoads[slaveId] = slaveLoad;

    	   cout << "slaveId  "<< slaveId << "  currentLoad "<< currentLoads[slaveId] << endl;
			
	   // if distributed, then the power/ slaveLoad 
	   if(useLoad)
		availablePowers[slaveId] = (powers[slaveId]/slaveLoad);//
	   else
		availablePowers[slaveId]= (powers[slaveId]);//

	   totalPower += availablePowers[slaveId];
	   cout << "totalPower = "<< totalPower<<" Available power "<<  availablePowers[slaveId]<<endl;

	}
	

	/* Start the FSS scheme */
	outFile << "Factorial scheme" << endl;
	outFile << endl;

	if(totalPower == 0)
	{
		cerr << "Total available power is null!!!";
		exit(1);
	}

	totalTime1 = MPI_Wtime();

        //cout << "----------Master totalTime1----------: " << totalTime1 << endl;


	//calculating the chunks for each stage(round)

	roundNumber =  new int[noSlaves + 1];

	for(int i = 1; i < noSlaves + 1; i++)
		roundNumber[i] = 0;
	
	// Table to store the chunk size for each round	
	roundsTable = new long[MAXROUNDS*noSlaves + 1];
        candidateTable = new float[MAXROUNDS*noSlaves + 1];

        int remainingChunks =  DOTSX;
        float thisStageChunkSize; //the size of each chunk in this stage
        int chunkAssignedInThisStage; // the total chunk assigned in this stage
	int tempChunk;
	float newChunk;
        i = 0;// rounds

        // Round Table creation begun
        while(remainingChunks > 0)
        {
                int j;
                chunkAssignedInThisStage = 0;
		thisStageChunkSize = (float)(remainingChunks) / (float)(2*totalPower);

		/* Ceiling function */
                if (thisStageChunkSize-(int)thisStageChunkSize>0)
                       tempChunk = (int)thisStageChunkSize + 1;
                else
                       tempChunk = (int)thisStageChunkSize;

		if(threshold < tempChunk)
		{
			for(j = 1; j <= noSlaves; j++)
			{
				// computing chunk for each slave in round i 
			        thisStageChunkSize = (float)(remainingChunks) / (float)(2*totalPower);

				// Storing candidate chunk size for each slave in round i to the Table 
			   	candidateTable[i*noSlaves + j] = thisStageChunkSize;
			}
		}

		else // tempChunk <= threshold
		{
                        for(j = 1; j <= noSlaves; j++)
                        {
                              // Giving the chunk size as threshold
                              currentChunk = (int)(threshold);

                              candidateTable[i*noSlaves + j] = (float)currentChunk;
                        }
		}


            for(j = 1; j<= noSlaves; j++)
            {
                if(remainingChunks > (totalPower*candidateTable[i*noSlaves + j]))
                {
			if((candidateTable[i*noSlaves + j]) == threshold)
				chunkThreshCount++; //The number of chunks with size threshold

                       	thisStageChunkSize = candidateTable[i*noSlaves + j]*availablePowers[j];
 
			/* Ceiling function */
                	if (thisStageChunkSize-(int)thisStageChunkSize>0)
                       		tempChunk = (int)thisStageChunkSize + 1;
                	else
                       		tempChunk = (int)thisStageChunkSize;

			//storing chunk for each slave for the round i in the roundTable 
                      	roundsTable[i*noSlaves + j] = tempChunk;

                }
                else
		{
		     /* Dividing the remaining chunks to all the slaves, equally if fss or according to power if dfss */

		     thisStageChunkSize = (float)(remainingChunks*availablePowers[j]) / (float)(totalPower);
					
		     //Here we are taking the floor of the computed chunk	
		     currentChunk = (int)thisStageChunkSize;	

		     // storing chunk for each slave for the round i in the Table
               	     roundsTable[i*noSlaves + j] = (long)currentChunk;

		     // Total chunk assigned in the round j
                     //chunkAssignedInThisStage += roundsTable[i*noSlaves + j];

		}//End of IF-ELSE

	        // Total chunk assigned in this stage
                chunkAssignedInThisStage += roundsTable[i*noSlaves+j];

           }// End of for

           // The remaining chunks
	   remainingChunks -= chunkAssignedInThisStage;

	   int stage = i;//current stage
	   if(remainingChunks < 0)
		remainingChunks = 0;
	   else
	   {
                if(remainingChunks <threshold)
                {
                       stage++;
                       for(j=1;j<=noSlaves;j++)
                       {
                              roundsTable[stage*noSlaves + j] = (long)remainingChunks;
                              remainingChunks = 0;

                       } //End of For
                }
	   }//End of Else

	   i++; // incrementing the stage

        } // End of While


        // End of round table creation 


	assignedColumns = 0;
	unassignedColumns = DOTSX - assignedColumns;
	FinalReport finalReport;
	
       	int counter=9;
       	int noEntry=0;
       	
       
       	float scomm[noSlaves + 1];
       	
       	for(int i=1; i<=noSlaves; i++)
       		scomm[i] = 0;
       	
       	float tempCommTime1 = 0;
       	float tempCommTime = 0;
       		
	int size = 2*DOTSX*DOTSY/totPower;	
	//getData = (float*) calloc (20, sizeof(float));

	getData = new float[size+10];	
			
              	
	while (1) 
	{
    	        // Recieve work request or final report from the slaves 	
      		MPI_Recv(getData, size+10, MPI_FLOAT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status); 
      		
      		slaveId = (int)getData[0];
		currentLoad = (int)getData[1];
		
		scomm[slaveId] = MPI_Wtime();
		
				
					
	    	if(getData[2] == 1) //slave already sent final report
	    	{
			
			
			saveData[0][slaveId] = (int)getData[0];
			
			for(int i=1;i<=5;i++)
				saveData[i][slaveId] = getData[i+2];
				
			
			if(saveData[4][slaveId] > maxcommtime)
				maxcommtime = saveData[4][slaveId];
			
		
			endCount++;
	   	
	
			if(endCount == noSlaves)break;
	    	}
	    	else //start X="compute and send chunk to slave"
	    	{
			
			/* calling the work calculate function to allocate the work to the slave */
			workCalculate(slaveId);

				
			//cout << "slave " << slaveId << " : " << "start Col: "<<work[0] << " - " << "Number of Columns: "<< work[1] << endl;
			
			tempCommTime1 = MPI_Wtime();
			tempCommTime = tempCommTime1 - scomm[slaveId];
			
			work[3] = (int)(tempCommTime*1000);
			
			MPI_Send(work, 4, MPI_INT, slaveId, 4, MPI_COMM_WORLD);
			
					
		
	   }//X ends
	}// while ends

	

	totalTime2 = MPI_Wtime();

	//printing the round table 
		outFile << "Rounds table : " << endl;
                cout << "Stages : "<< endl;
                /* print the chunk size for each round */
                for(int ii = 0; ii <= i; ii++)
                {
                        outFile << "Round "<< ii <<": ";
                        cout << "stage "<< ii <<": ";
                        for(int j = 1; j < noSlaves+1; j++)
                        {
                                outFile << '\t' << roundsTable[ii*noSlaves + j];
                                cout << '\t' << roundsTable[ii*noSlaves +j];
                        }
                        outFile << endl;
                        cout << endl;
                }

	cout<<"Number of scheduling steps:"<<chunkCount<< endl;
	cout<<"Number of chunks with same size:"<<chunkThreshCount<< endl;

	outFile<<"Number of scheduling steps:"<<chunkCount<< endl;
        outFile<<"Number of chunks with same size:"<<chunkThreshCount<< endl;

	cout << "----------Master totalTime2----------: " << totalTime2 << endl;
	
	totalTime = totalTime2 - totalTime1;
	
	cout << "----------Master totalTime-----------: " << totalTime << endl;
	
	cout<<"totalMasterTime: "<<totalTime<<endl;	
	
		
	for(int i=1;i<=noSlaves;i++)
		slaveDone(i);
		
		
}

void Master::slaveDone(int slaveId)
{
	static int slavesDone = 0;
	static double totalcommtime = 0;
	static double totalcomputetime = 0;
	static double totalTimeFromSlave = 0;
	static double totalWaitCommTime = 0;
	outFile << endl << "Slave " << saveData[0][slaveId] << " ( power " << virtualPowers[slaveId] << " ) report" << endl << endl;
	outFile << "Computed columns: " << saveData[1][slaveId] << endl;
	outFile << "Number of requests: " << saveData[2][slaveId] << endl;

	char buf[100];
	print_mili_time((long)(saveData[3][slaveId]*1000), buf);
	outFile << "Total time: " << buf << endl;

        totalTimeFromSlave += saveData[3][slaveId];

	print_mili_time((long)(saveData[4][slaveId]*1000), buf);
	outFile << "Comm time: " <<buf <<endl;
		
	totalcommtime += saveData[4][slaveId];

	print_mili_time((long)(saveData[5][slaveId]*1000), buf);
	outFile << "Compute time: " << buf << endl;
	
	if(saveData[5][slaveId] > maxcomputetime)
		maxcomputetime = saveData[5][slaveId];
	
	totalcomputetime += saveData[5][slaveId];

	totalWaitCommTime = saveData[3][slaveId] - saveData[5][slaveId];

        print_mili_time((long)(totalWaitCommTime*1000), buf);
        outFile << "Total Wait and Commn Time: " << buf << endl;

	//cout<<totalcomputetime<<endl;
	print_mili_time((long)((saveData[3][slaveId]-saveData[4][slaveId]-saveData[5][slaveId])*1000), buf);
	//outFile << "Other time: " << buf << endl << endl;


	if(slaveId == noSlaves)
	{
		outFile << endl << endl;
		
		print_mili_time((long)(totalTime*1000), buf);
		outFile << "Master total time: " << buf << endl;
	
		print_mili_time((long)(totalcommtime*1000), buf);
		outFile << "total communication time: " << buf << endl;
		
		//print_mili_time((long)(communicationtime*1000), buf);
		//outFile << "total communication time: " << buf << endl;
		
		print_mili_time((long)(maxcomputetime*1000), buf);
		outFile << "Maximum compute time: " << buf << endl;

		print_mili_time((long)(totalTimeFromSlave*1000), buf);
                outFile << "Total time from slave: " << buf << endl;
		
		//outFile <<"mili seconds for one colunm: "<< (float)totalcomputetime*1000/(float)DOTSX << endl;
	
		cout << "Master finished writing solution, exiting" << endl;

		MPI_Finalize(); 
		exit(0);
	}
}
