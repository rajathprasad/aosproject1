#include <stdio.h>
#include "gtthread.h"

gtthread_mutex_t fork1, fork2, fork3, fork4, fork5;
gtthread_t p1,p2,p3,p4,p5;

void* philosopher(char c)
{
	printf("bla\n");
	while(1)
	{
		if ( c == 'A')
		{
			printf("Philosopher A is hungry\n");
			printf("Philosopher A tries to acquire fork #1\n");
			gtthread_mutex_lock(&fork1);
			printf("Philosopher A tries to acquire fork #5\n");
			gtthread_mutex_lock(&fork5);
			printf("Philosopher A is eating.\n");
			gtthread_yield();
			sleep(1);
			gtthread_mutex_unlock(&fork5);
			printf("Philosopher A releases fork #5\n");
			gtthread_mutex_unlock(&fork1);
			printf("Philosopher A releases fork #1\n");
			printf("Philosopher A is resting.\n");
			gtthread_yield();
			sleep(1);
		}
		else if ( c == 'B')
		{
			printf("Philosopher B is hungry\n");
			printf("Philosopher B tries to acquire fork #1\n");
			gtthread_mutex_lock(&fork1);
			printf("Philosopher B tries to acquire fork #2\n");
			gtthread_mutex_lock(&fork2);
			printf("Philosopher B is eating.\n");
			gtthread_yield();
			sleep(1);
			gtthread_mutex_unlock(&fork2);
			printf("Philosopher B releases fork #2\n");
			gtthread_mutex_unlock(&fork1);
			printf("Philosopher B releases fork #1\n");
			printf("Philosopher B is resting.\n");
			gtthread_yield();
			sleep(1);	
		}		
		else if ( c == 'C')
		{
			printf("Philosopher C is hungry\n");
			printf("Philosopher C tries to acquire fork #2\n");
			gtthread_mutex_lock(&fork2);
			printf("Philosopher C tries to acquire fork #3\n");
			gtthread_mutex_lock(&fork3);
			printf("Philosopher C is eating.\n");
			gtthread_yield();
			sleep(1);
			gtthread_mutex_unlock(&fork3);
			printf("Philosopher C releases fork #5\n");
			gtthread_mutex_unlock(&fork2);
			printf("Philosopher C releases fork #1\n");
			printf("Philosopher C is resting.\n");
			gtthread_yield();
			sleep(1);
		}
		else if ( c == 'D')
		{
			printf("Philosopher D is hungry\n");
			printf("Philosopher D tries to acquire fork #3\n");
			gtthread_mutex_lock(&fork3);
			printf("Philosopher D tries to acquire fork #4\n");
			gtthread_mutex_lock(&fork4);
			printf("Philosopher D is eating.\n");
			gtthread_yield();
			sleep(1);
			gtthread_mutex_unlock(&fork4);
			printf("Philosopher D releases fork #4\n");
			gtthread_mutex_unlock(&fork3);
			printf("Philosopher D releases fork #3\n");
			printf("Philosopher D is resting.\n");
			gtthread_yield();
			sleep(1);
		}
		else
		{
			printf("Philosopher E is hungry\n");
			printf("Philosopher E tries to acquire fork #4\n");
			gtthread_mutex_lock(&fork4);
			printf("Philosopher E tries to acquire fork #5\n");
			gtthread_mutex_lock(&fork5);
			printf("Philosopher E is eating.\n");
			gtthread_yield();
			sleep(1);
			gtthread_mutex_unlock(&fork5);
			printf("Philosopher E releases fork #5\n");
			gtthread_mutex_unlock(&fork4);
			printf("Philosopher E releases fork #4\n");
			printf("Philosopher E is resting.\n");
			gtthread_yield();
			sleep(1);
		}
	}
	return;
}


main()
{
	gtthread_init(10);
	gtthread_mutex_init(&fork1);
	gtthread_mutex_init(&fork2);
	gtthread_mutex_init(&fork3);
	gtthread_mutex_init(&fork4);
	gtthread_mutex_init(&fork5);
	printf("hello\n");

	gtthread_create(&p1,philosopher, (void *)'A');	
	gtthread_create(&p2,philosopher, (void *)'B');
	gtthread_create(&p3,philosopher, (void *)'C');
	gtthread_create(&p4,philosopher, (void *)'D');
	gtthread_create(&p5,philosopher, (void *)'E');

	gtthread_exit(NULL);
}
