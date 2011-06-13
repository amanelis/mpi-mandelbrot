//TSS-1D

//Master with receiving resuilt back from slave
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
	
	FChunk=1;
	previousChunk = 0;
	prevAPower=1;
	ngoal=0;
        chunkThreshCount = 0;
        chunkCount = 0;
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

void Master::workCalculate(int slaveId)
{
	
	startCol = assignedColumns;
        int totalChunks;
        int candidateChunk;
        int remainder = unassignedColumns;
			
        if(useLoad == 1)//if dtss
        {
           totalChunks = 0;
           for(int i=0;i<availablePowers[slaveId];i++)
           {
               if(FChunk) // if first chunk
               {
                  currentChunk = (int)(firstChunk);
	          FChunk=0; 
               }
               else
               {
                  if(threshold < (previousChunk - delta))
                       candidateChunk = (int)(previousChunk - delta);
                  else
                       candidateChunk = threshold;

                  if(remainder > candidateChunk){
                       currentChunk = candidateChunk;
                       if(currentChunk == threshold){
                            chunkThreshCount++;
                       }
                  }
                  else
                       currentChunk = remainder;
                }
                previousChunk = currentChunk;
                totalChunks += currentChunk; // total chunks assigned to the current slave 
                remainder = remainder - currentChunk;
                if(remainder == 0)
                    break;
             }//End of For
      
             ngoal = totalChunks;
             unassignedColumns = remainder;
          }
          else // if tss
          {
              if(FChunk){//if first chunk
                 ngoal = (int) (firstChunk);
                 FChunk = 0;
              }
              else
              {
                 if(threshold < (previousChunk -  delta))
                        candidateChunk = (int)(previousChunk - delta);
                 else{
                        candidateChunk = threshold;
                 }
                 if(unassignedColumns > candidateChunk){
                         if(candidateChunk ==  threshold){
                             chunkThreshCount++;
                         }
                         ngoal = candidateChunk;
                 }
                 else
                         ngoal = unassignedColumns;
              }
              previousChunk = ngoal;
              unassignedColumns =  unassignedColumns - ngoal;
        } // End of IF TSS 
                
	
	assignedColumns += ngoal;
        prevAPower=availablePowers[slaveId];
		        
	if(nonded==1)
	     	work[2]=1;
	        else
		work[2]=0;
				
	work[0] = startCol;
	work[1] = ngoal;
	
        if(ngoal > 0)
            chunkCount++;

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
	//Chunk chunk;
	int endCount = 0;
	float goal;
	
	float temp,tempfirstchunk;
	
	maxcomputetime = 0;
	communicationtime = 0;
	maxcommtime = 0;
	
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
	

	/* Start the trapezoid  scheme */
	outFile << "Trapezoid  scheme" << endl;
	outFile << endl;

	if(totalPower == 0)
	{
		cerr << "Total available power is null!!!";
		exit(1);
	}

	totalTime1 = MPI_Wtime();

        //cout << "----------Master totalTime1----------: " << totalTime1 << endl;


	//apply TSS formulas to find parameters.
	tempfirstchunk = (float)(DOTSX / (2.0 * totalPower));
	
	firstChunk=(int)tempfirstchunk;

	//cout<<"Temp first chunk: " << tempfirstchunk <<" First Chunk: "<< firstChunk << endl;
		
        //outFile <<"First Chunk:"<<firstChunk<< endl;

	lastChunk =threshold;
	
	//NO. OF CHUNK IS CEILING OPERATION
	
	temp=float(2 * DOTSX / (firstChunk + lastChunk));
	if (temp-(int)temp>0)
		noChunks=(int)temp+1;
	else
		noChunks=(int)temp;
		

	if(noChunks != 0)
		delta =(int)((firstChunk - lastChunk)/(noChunks-1));
   
        outFile <<"delta:"<<delta<< endl;
			
	realChunk = firstChunk;
	currentChunk = (int)firstChunk;

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

       	int size = maxPower*(DOTSX*DOTSY)/(2*totPower);	
	
	getData = new float[size+10];
	
	//getData = (float*) calloc (size+10, sizeof(float));
              	
	while (1) 
	{
    		
      		MPI_Recv(getData, size+10 , MPI_FLOAT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status); 
      		
      		slaveId = (int)getData[0];
		currentLoad = (int)getData[1];
		
		scomm[slaveId] = MPI_Wtime();
		
				
					
	    	if(getData[2] == 1) //slave already sent final report
	    	{
			
			
			saveData[0][slaveId] = (int)getData[0];
			
			for(int i=1;i<=7;i++)
				saveData[i][slaveId] = getData[i+2];
				
			if(saveData[4][slaveId] > maxcommtime)
				maxcommtime = saveData[4][slaveId];
			
		
			endCount++;
	   	
	
			if(endCount == noSlaves)break;
	    	}
	    	else //start X="compute and send chunk to slave"
	    	{
			

			workCalculate(slaveId);

				
			//cout << "slave " << slaveId << " : " << work[0] << " - " << work[1] << endl;
			
			tempCommTime1 = MPI_Wtime();
			tempCommTime = tempCommTime1 - scomm[slaveId];
			
			work[3] = (int)(tempCommTime*1000);
			
			MPI_Send(work, 4, MPI_INT, slaveId, 4, MPI_COMM_WORLD);
			
					
		
	   }//X ends
	}// while ends

	

	totalTime2 = MPI_Wtime();

        cout << "Number of scheduling steps:"<<chunkCount<< endl;
        cout << "Number of same chunk size scheduling steps:"<<chunkThreshCount<< endl;

        outFile << "Number of scheduling steps:"<<chunkCount << endl;
        outFile <<"Number of same chunk size scheduling steps:"<<chunkThreshCount<< endl;

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
		//outFile << "total compute time: " << buf << endl;
		outFile << "Maximum compute time: " << buf << endl;

		//outFile <<"mili seconds for one colunm: "<< (float)totalcomputetime*1000/(float)DOTSX << endl;
	
		cout << "Master finished writing solution, exiting" << endl;

		MPI_Finalize(); 
		exit(0);
	}
}
