/************************************************************
 *  File:
 *  Author:
 *  Version :
 *  Date:
 *  Description:
 *  History:
 *            <Zhou Xiaomin> <2016-01-20>
 ***********************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

int order_thread_flag;
pthread_t order_thread;
void order_thread_task(void);

int init_order(void);
int uninit_order(void);

int init_order(void) {

	pthread_attr_t attr;
	order_thread_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&order_thread, &attr, (void *) order_thread_task, NULL);
	pthread_attr_destroy(&attr);
	if (order_thread == 0) {
		printf("don't create order thread \n");
		return (-1);
	}
	return 0;
}

int uninit_order(void) {
	//order
	order_thread_flag = 0;
	usleep(40 * 1000);
	return 0;
}

void order_thread_task(void) {

	int timenum;

	printf("create order thread :0.01\n");

	timenum = 0;
	while (order_thread_flag) {

		timenum++;

		if (timenum > 0xFFFFFF) {
			timenum = 0;
		}
		usleep((20-10)*1000);
	}
}

