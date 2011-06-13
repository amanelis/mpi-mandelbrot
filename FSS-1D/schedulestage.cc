//schedulestage.cc
#include "schedule.h"
#include "schedule_aux.h"
#include "mpi.h"
#include <unistd.h>
#include <iostream.h>
#include <fstream.h>
#include "inclimit.c"
#include "input.h"

int main(int argc, char** argv)
{
	inclimit();
	
	int myId;
	MPI_Status status;
	char ch[] = "load2";	

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&myId);
	
	char name[10];
	gethostname(name, 10);

	if(myId == 0)
	{
		cout << "Starting master " << name << endl;
		Master* master = new Master();
		master->slaveCall();
		delete(master);
	}
	else
	{ 	
		
		cout << "My id is " << myId << " name " << name << endl;
		Slave slave(myId);
	}		

       MPI_Finalize(); 
}




