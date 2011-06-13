//schedule.h
#ifndef SCHEDULE_H
#define SCHEDULE_H

	#include "timing.h"
	#include<stdlib.h>
	#include<string.h>
	#include <fstream.h>
	#include <stdio.h>
	#include "mpi.h"
	#include <math.h>
	static const int DOTSX = 8000 ;  //size of mandelbrot
	static const int DOTSY = 8000;  //size of mandelbrot

	typedef int Columns[DOTSX];
	typedef long* ChunkResult;


	struct Chunk
	{
		int startColumn;
		int noOfColumns;
	};


	struct FinalReport
	{
		int slaveId;

		int computedColumns;
		int noOfRequests;

		double totalTime;
		double commTime;
		double computeTime;
		
	};


	class MasterProxy
	{
           public:
		void registerSlave(int slaveId, int slaveLoad);
		void slaveDone(FinalReport&);
		Chunk* requestWork(int slaveId, int currentLoad, FinalReport&);
	};	


	class Slave
	{
		struct timeb totalTimeStart, totalTimeStop;
		struct timeb commTimeStart, commTimeStop;
		struct timeb finalCommTimeStart, finalCommTimeStop;
		struct timeb computeTimeStart, computeTimeStop;
		FinalReport finalReport;
                Columns columns;

		long computeMandelbrot(int row, int col);
		long calcPixel(float cA, float cBi);
		

	   public:
		MasterProxy masterProxy;

		Slave(int myId);
		void startWorking();
	};

	class Master
	{
	   protected:
		//int noSlaves;
		char** slaveNames;
		int* currentLoads;
		float* virtualPowers;
		float* availablePowers;
		float* remPwr;
		int* slaveWaitTimes;
		float* powers;
		int* slaveEnd;
		double saveData[6][32];
		//int type;
		const static int MAXROUNDS = 30;
		float totalVirtualPower;
		long* roundsTable;
		int* roundNumber;
	 	int chunkSize;
		//int distribution=1;	// 0 = normal, 1 = round robin
		//int robinFactor;
		//Columns columns;// the permutation from round robin
		float **resultMatrix;
		//long resultMatrix[DOTSY][DOTSX];
		int computedColumns;
		int assignedColumns;
		int unassignedColumns;
		
		struct timeb totalTimeStart, totalTimeStop;
		double totalTime;
		double totalTime1, totalTime2;
		
		float maxcomputetime;
		float communicationtime;
		float maxcommtime;

		ofstream outFile;

		float totalPower;
		float noChunks;

		float delta;
		float realChunk;
		float correction;

		float requestNumber;
		int currentChunk;
		float bump;

		int changedLoad;
		
		int firstStage;
		int firstStageSlaves;
		
		int isched;
		int work[4];
		int nchx;
		int vx1[1000], vx2[1000];
		
		int nchy;
		int vy1[1000], vy2[1000];
		
		int nmax, nmin;
		int ipoint, jpoint;
		int last;
		int chunkCount, chunkThreshCount;
		
		void workCalculate();

		void saveResult();
		void configureMaster();
		int newChunkSize(int slaveNumber);

		void requestWork();
		void slaveDone(int slaveId);

	   public:
		Master();
		~Master();
		void slaveCall();
	};

#endif
