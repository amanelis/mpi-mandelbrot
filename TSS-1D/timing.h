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

#ifndef __TIMING__H
#define __TIMING__H

	#include 	<time.h>
	#include 	<sys/timeb.h>

	void reset_timer(struct timeb* timer);
	void elapsed_time(struct timeb end_time, struct timeb begin_time, 
				int* h, int* m, int* s, int* ms);
	void print_elapsed_time(struct timeb end_time, struct timeb start_time, char* buf);
	long get_elapsed_time(struct timeb end_time, struct timeb start_time);
	void print_mili_time(long time, char* buf);

#endif
