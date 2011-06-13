/*
	struct timeb start_time, end_time;
	int hours, mins, secs, ms;

    	reset_timer(&start_time);
    	reset_timer(&end_time);

	ftime(&start_time);

	... activity to measure ...

	ftime(&end_time);

   	elapsed_time(end_time, start_time, &hours, &mins, &secs, &ms);
	printf("Total time: %dh%dm%ds%dms\n", hours, mins, secs, ms);

		OR

	char buf[1000];

	struct timeb start_time, end_time;
	ftime(&start_time);
	... activity to measure ...
	ftime(&end_time);

	print_elapsed_time(end_time, start_time, buf);
	printf("Total time: %s\n", buf);

*/

#include 	<time.h>
#include 	<sys/timeb.h>
#include	<stdio.h>

void reset_timer(struct timeb* timer)
{
  timer->time = 0;
  timer->millitm = 0;
}


void elapsed_time(struct timeb end_time, struct timeb begin_time, 
				int* h, int* m, int* s, int* ms)
{
  double diff;

  *h = *m = *s = *ms = 0;
  diff = difftime(end_time.time, begin_time.time);
  *ms = end_time.millitm - begin_time.millitm;
  if (*ms < 0) {
    *ms += 1000;  
    diff--;       
  }

  *s = (long) diff % 60;        
  *m = ((long) diff / 60) % 60; 
  *h = (long) diff / 3600;
} 

void print_elapsed_time(struct timeb end_time, struct timeb start_time, char* buf)
{
	int hours, mins, secs, ms;

   	elapsed_time(end_time, start_time, &hours, &mins, &secs, &ms);

	if(hours)
		sprintf(buf, "%dh %dm %ds %dms", hours, mins, secs, ms);
	else if(mins)
		sprintf(buf, "%dm %ds %dms", mins, secs, ms);
	else if(secs)
		sprintf(buf, "%ds %dms", secs, ms);
	else
		sprintf(buf, "%dms", ms);
}

/* Returns in miliseconds */

long get_elapsed_time(struct timeb end_time, struct timeb start_time)
{
	int hours, mins, secs, ms;

   	elapsed_time(end_time, start_time, &hours, &mins, &secs, &ms);

	return (hours*3600000 + mins*60000 + secs*1000 + ms);
}

void print_mili_time(long time, char* buf)
{
	int hours, mins, secs, ms;

	hours = time/3600000;
	time = time%3600000;

	mins = time/60000;
	time = time%60000;

	secs = time/1000;
	ms = time%1000;

	if(hours)
		sprintf(buf, "%dh %dm %ds %dms", hours, mins, secs, ms);
	else if(mins)
		sprintf(buf, "%dm %ds %dms", mins, secs, ms);
	else if(secs)
		sprintf(buf, "%ds %dms", secs, ms);
	else 
		sprintf(buf, "%dms", ms);
}
