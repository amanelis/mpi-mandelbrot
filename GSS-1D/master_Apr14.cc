//GSS-1D

//Master receives the resutls from slaves
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
	
	/*
	if(type == 0)
	{
	    outFile << "Chunk size is " << chunkSize << endl;
	    outFile << "First chunk size is " << firstChunk << endl;
	    outFile << "Last chunk size is " << lastChunk << endl;
	}
	*/

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
	Chunk chunk;
	int endCount = 0;
	float goal;
	
	float temp, tempfirstchunk;
	int tempChunk;
	
	maxcomputetime = 0;
	communicationtime = 0;
	maxcommtime = 0;
        chunkThreshCount = 0;
        chunkCount = 0;
	
	////float *getData;
	////getData=new float[DOTSX*DOTSY];
	float *getData;
		
	
	
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
	
	if(useLoad == 1)
	{	
		while(machinePower[count-1]){
			cout<<"Machinepower"<<machinePower[count-1]<<endl;
			powers[count]=machinePower[count-1];
			count++;
		}		 	
	}


	//REMOVE B_CAST FOR useLoad AND PUT IN input.h
	
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
	  ///  currentLoads[slaveId] = 1;
    	   cout << "slaveId  "<< slaveId << "  currentLoad "<< currentLoads[slaveId] << endl;
			
	   if(useLoad)
		availablePowers[slaveId] = (powers[slaveId]/slaveLoad);//
	   else
		availablePowers[slaveId]= (powers[slaveId]);//
	   totalPower += availablePowers[slaveId];
	   cout << "totalPower = "<< totalPower<<" Available power "<<  availablePowers[slaveId]<<endl;
	}
	

	/* Start the GSS scheme */
	outFile << "GSS scheme" << endl;
	outFile << endl;

	if(totalPower == 0)
	{
		cerr << "Total available power is null!!!";
		exit(1);
	}


	totalTime1 = MPI_Wtime();

        //cout << "----------Master totalTime1----------: " << totalTime1 << endl;


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
       		
        int size = maxPower*(DOTSX*DOTSY)/(totPower);	

	//getData = (float*) calloc (20, sizeof(float));
	getData = new float[DOTSX*DOTSY];

              	
	while (1) 
	{
    		
    		
      		MPI_Recv(getData, size+100, MPI_FLOAT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status); 
      		
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
		    int totalChunks = 0;		
		    int Remainder = unassignedColumns;
		    int candidateChunk = 0;	
                    currentChunk = 0;
		 if(availablePowers[slaveId] >= 1){

		    chunk.startColumn =  assignedColumns;

		    if(useLoad == 1) // if dgss
		    {
			for(int i=1; i<=availablePowers[slaveId]; i++) 
			{
			    temp = (float)(Remainder) / (float)(totalPower);
			    
			    //ceiling function
			    if(temp-(int)temp > 0)
				tempChunk = (int)temp +1;
		            else
				tempChunk = (int)temp;

                            if(threshold < tempChunk)
                                candidateChunk = tempChunk;
                            else
                                candidateChunk = threshold;
            
                            if(Remainder > candidateChunk){
                                if(candidateChunk == threshold)
                                      chunkThreshCount++;

                                currentChunk = candidateChunk;
                            }
                            else
                                currentChunk = Remainder;

			    Remainder = Remainder - currentChunk;// Remaining chunks
			    totalChunks += currentChunk; // total chunk assigned for this slave
                            if(Remainder == 0)
                                break;
			}
			chunk.noOfColumns = totalChunks;
                        unassignedColumns = Remainder;
		    }
		    else // if gss
	            {
               		 temp = (float)(unassignedColumns) / (float)(totalPower);
			// ceiling function
               		 if ( temp-(int)temp > 0)
               		        tempChunk = (int)temp +1;
               		 else
                        	tempChunk = (int)temp;

			  if( threshold < tempChunk)
				candidateChunk = tempChunk; 
			  else
				candidateChunk = threshold;

			  if(unassignedColumns > candidateChunk)
                          {
                                if(candidateChunk == threshold)
                                     chunkThreshCount++;

				chunk.noOfColumns = candidateChunk;
                          }
			  else
				chunk.noOfColumns = unassignedColumns;

                          unassignedColumns = unassignedColumns - chunk.noOfColumns;
		    }

			assignedColumns += chunk.noOfColumns;
        		work[0] = chunk.startColumn;
        		work[1] = chunk.noOfColumns;
			work[2] = 0; // not non-dedicated

			if(work[1] > 0){
				chunkCount++;
			}

		   }

			//cout << "slave " << slaveId << " : " << work[0] << " - " << work[1] << endl;
			
			tempCommTime1 = MPI_Wtime();
			tempCommTime = tempCommTime1 - scomm[slaveId];
			
			work[3] = (int)(tempCommTime*1000);
			
			MPI_Send(work, 4, MPI_INT, slaveId, 4, MPI_COMM_WORLD);
			
					
		
	   }//X ends
	}// while ends

	

	totalTime2 = MPI_Wtime();

	cout << "The number of scheduling steps: "<<chunkCount << endl;
        cout << "The number of same chunk scheduling steps: "<<chunkThreshCount<< endl;
        outFile << "The number of scheduling steps: "<<chunkCount << endl;
        outFile << "The number of same chunk scheduling steps: "<<chunkThreshCount<< endl;

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
	outFile << endl << "Slave " << saveData[0][slaveId] << " ( power " << virtualPowers[slaveId] << " ) report" << endl << endl;
	outFile << "Computed columns: " << saveData[1][slaveId] << endl;
	outFile << "Number of requests: " << saveData[2][slaveId] << endl;

	char buf[100];
	print_mili_time((long)(saveData[3][slaveId]*1000), buf);
	outFile << "Total time: " << buf << endl;
	
	print_mili_time((long)(saveData[4][slaveId]*1000), buf);
	outFile << "Comm time: " <<buf <<endl;
		
	totalcommtime += saveData[4][slaveId];

	print_mili_time((long)(saveData[5][slaveId]*1000), buf);
	outFile << "Compute time: " << buf << endl;
	
	if(saveData[5][slaveId] > maxcomputetime)
		maxcomputetime = saveData[5][slaveId];
	
	totalcomputetime += saveData[5][slaveId];
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
		//outFile << "total compute time: " << buf << endl;
		outFile << "Maximum compute time: " << buf << endl;
		
		//outFile <<"mili seconds for one colunm: "<< (float)totalcomputetime*1000/(float)DOTSX << endl;
	
		cout << "Master finished writing solution, exiting" << endl;

		MPI_Finalize(); 
		exit(0);
	}
}
