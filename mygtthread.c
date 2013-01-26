#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define MAX_THREADS 1000

struct gtthread_t
{
	long int id;
	struct gtthread_t* next;  //pointer to next node
	int terminted;	
}

/*global declarations*/
struct gtthread_t gtarray = (gtthread_t*)malloc(MAX_THREADS*sizeof(struct gtthread_t));  // array of threads
struct gtthread_t *head; // points to head of linked list
struct gtthread_t *tail; // points to tail of linked list
struct gtthread_t *current; //points to currently executing thread
int count=0;  //number of threads in list (active threads)
struct gtthread_t *main_t; // pointer to the main thread, active throughout the program
long period_t = 0; // time slice of threads, initially set to 0
long gt_id = 0; // counter to assign id's to threads

/* function declarations */

void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);

void add_thread(struct gtthread_t *thread)
{
	if(count==0) // no gtthread active
	{
		head = thread;
		tail = thread;
	}
	else  // add it to end of queue
	{
		tail->next = thread;
		thread->next = head;
		tail = thread; 
	}
	count++; // increment count of active threads
}

void gtthread_init(long period)
{
	period_t = period;
	main_t = &gtarray[0];
	main_t->id=gt_id;
	main_t->next= NULL;
	gt_id++;
	current = main_t;
	add_thread(main_t);	
}

int gtthread_create(gtthread_t *thread, void *(*start_routine)(void *),void *arg)
{
	struct gtthread_t *new_t = &gtarray[gt_id];
	new_t->id = gt_id;
	gt_id++
	new_t->next = NULL;
	thread = new_t;
	char stack1[16384];

	getcontext(&new_t->context);
	ctxt_t1.uc_stack.ss_sp = stack1;
        ctxt_t1.uc_stack.ss_size = sizeof(stack1);
        ctxt_t1.uc_link = 0; // why???

	add_thread(new_t);

}


int main()
{



}

