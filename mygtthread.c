#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

#define MAX_THREADS 1000


struct gtthread_t 
{
	long int id;
	struct gtthread_t* next;  //pointer to next node
	struct gtthread_t* parent;
	int terminated;
	ucontext_t context;	
	long int num_children;
	void *retval;

}gtarray[MAX_THREADS];

/*global declarations*/
//struct gtthread_t gtarray = (gtthread_t*)malloc(MAX_THREADS*sizeof(struct gtthread_t));  // array of threads
struct gtthread_t *head; // points to head of linked list
struct gtthread_t *tail; // points to tail of linked list
struct gtthread_t *current; //points to currently executing thread
int count=0;  //number of threads in list (active threads)
struct gtthread_t *main_t; // pointer to the main thread, active throughout the program
long period_t = 0; // time slice of threads, initially set to 0
long gt_id = 0; // counter to assign id's to threads

volatile sig_atomic_t keep_going = 1;

struct itimerval timer;
ucontext_t ctxt_main;
sigset_t block;

/* function declarations */
void swap_thread();
void init_timer(long period);
void gtthread_init(long period);
int  gtthread_create(struct gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);

int  gtthread_join(struct gtthread_t thread, void **status);

void gtthread_exit(void *retval);
void gtthread_yield(void);
/*
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);*/
/*
void do_stuff (void)
     {
       puts ("Doing stuff while waiting for alarm....");
     }

while (keep_going)
         do_stuff ();
*/

void swap_thread()
{
	printf("in swap\n");	
	init_timer(period_t);
	swapcontext(&current->context,&current->next->context);
	current = current->next;
}


void init_timer(long period){
	long gt_reset_time=period;
	sigemptyset(&block);
	sigaddset(&block,SIGVTALRM);
	timer.it_value.tv_sec=-0;
	timer.it_value.tv_usec=period;
	timer.it_interval.tv_sec=0;
	timer.it_interval.tv_usec=period;
	printf("timer is %ld\n", period);
	signal(SIGPROF,swap_thread);
	setitimer(ITIMER_PROF,&timer,NULL);
	//sleep(10);
}


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

void delete_thread(struct gtthread_t *thread)
{
	struct gtthread_t *temp = head;
	struct gtthread_t *temp_del; //to delete

	//check if *thread is head or tail
	if(thread == head)
	{
		temp_del = head;
		temp_del->parent->num_children--;
		head = head->next;
		tail->next = head;
		delete(temp_del);
	}

	while(temp->next!= head)
	{
		if(temp->next->id = thread->id) //delete next thread
		{
			temp_del = temp->next;
			if(temp_del == tail)
				tail = temp;
			temp->next = temp->next->next;
			temp_del->parent->num_children--;
			delete(temp_del);
		}

	}

}

void gtthread_init(long period)
{
	period_t = period;
	main_t = &gtarray[0];
	main_t->id=gt_id;
	main_t->next= NULL;
	main_t->parent = main_t;
	main_t->num_children = 0;
	gt_id++;
	current = main_t;
	add_thread(main_t);
	init_timer(period_t);	
}

void gtthread_retarg(void *(*start_routine)(void *),void *arg)
{
	void *retval = start_routine(arg);
	gtthread_exit(retval);
}

int gtthread_create(struct gtthread_t *thread, void *(*start_routine)(void *),void *arg)
{
	struct gtthread_t *new_t = &gtarray[gt_id];
	new_t->id = gt_id;
	gt_id++;
	new_t->next = NULL;
	new_t->terminated= 0;
	new_t->parent = current;
	new_t->num_children = 0;
	current->num_children++;
	thread = new_t;
	char stack1[16384];
	getcontext(&new_t->context);
	new_t->context.uc_stack.ss_sp = stack1;
        new_t->context.uc_stack.ss_size = sizeof(stack1);
        new_t->context.uc_link = 0; // why???
	makecontext(&new_t->context, gtthread_retarg,2,start_routine,arg);
	add_thread(new_t);
	current = new_t;
}

int  gtthread_join(struct gtthread_t thread, void **status)
{
	long int temp_id = thread.id;	
	struct gtthread *temp = &gtarray[temp_id];
	while(temp->terminated!=1)
		gtthread_yield();
}

void gtthread_yield()
{
	sched_yield();
}

void gtthread_exit(void *retval)
{
	current->terminated = 1;	
	current->retval = retval;
	while(current->num_children>0)
		gtthread_yield();
	
	if(current = main_t)
		exit(0);
	delete_thread(current);


}


void hello()
{
	keep_going = 0;
	printf("woohoo\n");
}

void hello2()
{
	//keep_going = 0;
	printf("in hello2\n");
}

void main()
{
	struct gtthread_t t1, t2;
	gtthread_init(1);
	gtthread_create(&t1, hello, (void*)1000);
	gtthread_create(&t1, hello2, (void*)1000);
	printf("id is %d\n",current->id );	
	
	
	int i;
	for(i = 0; i < 10;i++)
	{
		printf("in main:%d\n", i);
	}
	//swapcontext(&ctxt_main,&gtarray[1].context);

	printf("bla\n");
	return;

}

