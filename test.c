/* testing */
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>

 volatile sig_atomic_t keep_going = 1;

struct gtthread_t
{
	long int id;
	struct gtthread_t* next;  //pointer to next node
	int terminted;	
}gt[2];
struct itimerval timer;
ucontext_t ctxt_main, ctxt_t1;
	
void func1()
{
	printf("func1:Swapped from main\n");
	int a = 5;
	swapcontext(&ctxt_t1, &ctxt_main);
	printf("func1:back in func1\n");
	printf("a is %d\n",a);
	return;
}

void
     do_stuff (void)
     {
       puts ("Doing stuff while waiting for alarm....");
     }

void hello()
{
	keep_going = 0;
	printf("woohoo\n");
}

void main()
{
	long int period = 1;
		
	char main_stack[16384];
	char stack1[16384];
	timer.it_value.tv_sec=0;
	timer.it_value.tv_usec=period;
	timer.it_interval.tv_sec=0;
	timer.it_interval.tv_usec=period;
	//setitimer(ITIMER_PROF,&timer,NULL);	
	printf("timer is %ld\n", period);
	signal(SIGVTALRM,hello);
	setitimer(ITIMER_VIRTUAL,&timer,NULL);	

	while (keep_going)
         do_stuff ();

	getcontext(&ctxt_t1);
	ctxt_t1.uc_stack.ss_sp = stack1;
        ctxt_t1.uc_stack.ss_size = sizeof(stack1);
        ctxt_t1.uc_link = &ctxt_main;
        makecontext(&ctxt_t1, func1, 0);
	printf("In main: entering\n");
	swapcontext(&ctxt_main, &ctxt_t1);
	
	printf("main: back in main\n");
	swapcontext(&ctxt_main, &ctxt_t1);
	printf("main:exiting main\n");
	return;
}
	

	



