C++FLAGS        =  -I. -mt -D_REENTRANT
SYSLIBS         = -Bdynamic -lnsl -lsocket
C++		= mpiCC -Wno-deprecated 

all: fss

fss: input.h power.h schedule.h schedulestage.cc schedule_aux.cc schedule_aux.h timing.cc timing.h slave.cc  master.cc
	$(C++) -o fss schedulestage.cc slave.cc schedule_aux.cc timing.cc master.cc

clean:
	rm *.o 
