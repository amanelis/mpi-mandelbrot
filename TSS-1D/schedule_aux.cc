	#include "schedule_aux.h"
	#include <stdlib.h>

/* 
   Returns the number of processes in the run-queue
   with the UNIX "w -u"
*/

	int getComputerLoad()
	{
		int n, load;
		float l;
		FILE*	f;
		char buf[40], info[100];
		char pattern[]="average:";
		char* tmp;

		/* Find load */
		strcpy(buf,"w -u > ");
		strcat(buf,"/tmp/sali");

		system(buf);
		f = fopen("/tmp/sali","r");
		n = fread(info,1,99,f);
		info[n] = '\0';
		tmp = (char*)strstr(info,pattern)+strlen(pattern) ;
		sscanf(tmp, "%f", &l);
		//printf("Load %f\n ",l);
		if(l<1.5) load=1;
			else load = (int)(l+1);
		fclose(f);

		return load;
	};
