// This FIXME fails to compile with -std=c99 for some reason. Works without
// #define __USE_ISOC11 1 didn't work either
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

int main() {
	struct timespec res,t1,t2;
	clockid_t clock_id = CLOCK_MONOTONIC;
	int ok = clock_getres(clock_id,&res);
	if(ok<0) {
		clock_id = CLOCK_REALTIME;
		ok = clock_getres(clock_id,&res);
		if(ok<0) {
			perror("No system clocks available");
			exit(-1);
		}
	}	

	printf("Clock type: %s\n", 
		clock_id == CLOCK_MONOTONIC ? 
		"CLOCK_MONOTONIC" : "CLOCK_REALTIME"); 
	printf("Reported Clock Resolution: %ldns\n",res.tv_nsec);
	clock_gettime(clock_id,&t1);
	clock_gettime(clock_id,&t2);
	printf("Real clock resolution: %ldns\n",t2.tv_nsec-t1.tv_nsec);

/*
// Maybe use these rather than timerfd?

	struct sigevent sevp;
	sevp.sigev_notify = SIGEV_SIGNAL;
	timer_t timer_id;
	ok = timer_create(clock_id, &sevp,
        		  &timer_id);
*/
	return(0);
}
