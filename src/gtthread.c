#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
//#include "helper_functions.h"

#define MAX_THREADS 1000

typedef unsigned long gtthread_t;
typedef unsigned long gtthread_mutex_t;

struct gtthread 
{
	gtthread_t id;
	struct gtthread* next;  //pointer to next node
	struct gtthread* parent;
	int terminated;
	ucontext_t context;	
	long int num_children;
	void *retval;

};

struct gtthread_mutex
{
	int lock;
	gtthread_t owner;
};

/*function declarations*/
void swap_thread();
void timer_start(long period);
void signal_block();
void signal_unblock();
void timer_start();
void add_thread(struct gtthread *thread);
void delete_thread(struct gtthread *thread);
void check_initialization();




/*global declarations*/
//struct gtthread thread_array = (gtthread*)malloc(MAX_THREADS*sizeof(struct gtthread));  // array of threads
struct gtthread thread_array[MAX_THREADS];
struct gtthread_mutex mutex_array[MAX_THREADS];
struct gtthread *head; // points to head of linked list
struct gtthread *tail; // points to tail of linked list
struct gtthread *current; //points to currently executing thread
int count=0;  //number of threads in list (active threads)
struct gtthread *main_t; // pointer to the main thread, active throughout the program
long period_t = 0; // time slice of threads, initially set to 0
long gt_id = 0; // counter to assign id's to threads
long mutex_id = 0; // counter to assign id's to mutexes
int t_kill;  //flag
struct itimerval timer;
ucontext_t ctxt_main, ctxt_dummy;
sigset_t signal_set;



void gtthread_yield(void)
{
	check_initialization();
	raise(SIGVTALRM);
}

void gtthread_exit(void *retval)
{
//	printf("in exit\n");
	check_initialization();
	current->terminated = 1;	
	current->retval = retval;
	if(current == main_t)
	{
		while(current->num_children>0)
			gtthread_yield();
		exit(0);
	}
	delete_thread(current);
	raise(SIGVTALRM);
}

void gtthread_init(long period)
{
	if(period <=0)
	{
		printf("Invalid period. Exiting.\n");
		exit(1);
	}	
	period_t = period;
	main_t = &thread_array[0];
	main_t->id=gt_id;
	main_t->next= NULL;
	main_t->terminated = 0;
	main_t->parent = main_t;
	main_t->num_children = 0;
	gt_id++;
	current = main_t;
	t_kill = 0;
	
	add_thread(main_t);
	timer_start(period_t);	
}

void gtthread_retarg(void *(*start_routine)(void *),void *arg)
{
	void *retval = start_routine(arg);
	gtthread_exit(retval);
}

int gtthread_create(gtthread_t *thread, void *(*start_routine)(void *),void *arg)
{
	check_initialization();
	struct gtthread *new_t = &thread_array[gt_id];
	new_t->id = gt_id;
	*thread = gt_id;
	gt_id++;
	
	new_t->next = NULL;
	new_t->terminated= 0;
	new_t->parent = current;
	new_t->num_children = 0;
	current->num_children++;
	//thread = new_t;
	getcontext(&new_t->context);
	new_t->context.uc_stack.ss_sp = malloc(16384);
        new_t->context.uc_stack.ss_size = 16384;
        new_t->context.uc_link = NULL; // thread exits
	sigemptyset(&new_t->context.uc_sigmask);
	new_t->context.uc_stack.ss_flags=0;
	makecontext(&new_t->context, (void(*)())gtthread_retarg,2,start_routine,arg);
	add_thread(new_t);
	return 0;
}

int  gtthread_join(gtthread_t thread, void **status)
{
	check_initialization();	
	struct gtthread *temp = &thread_array[thread];
	
	while(temp->terminated!=1)
		gtthread_yield();
	//printf("thread is %ld, return value is %d\n",thread, (int)temp->retval);
	if(status)
		*status = temp->retval;
	return 0;
}





gtthread_t gtthread_self(void)
{
	check_initialization();
	return current->id;
}

int  gtthread_equal(gtthread_t t1, gtthread_t t2)
{
	check_initialization();
	if(t1 == t2)
		return 1;
	else
		return 0;
}

int gtthread_cancel(gtthread_t thread)
{
	check_initialization();
	if(thread == current->id)
		gtthread_exit((void*)"GTTHREAD_CANCELED");
	else
	{
		struct gtthread *temp = &thread_array[thread];
		temp->terminated = 1;
		temp->retval = (void*)"GTTHREAD_CANCELED";		
		//while(temp->num_children>0)
		//	gtthread_yield();
		delete_thread(temp);
		
	}
	return 0;
}

int  gtthread_mutex_init(gtthread_mutex_t *mutex)
{
	*mutex = mutex_id;
	mutex_array[mutex_id].lock = 0; // currently unlocked;
	//mutex_array[mutex_id].owner = -1; // currently no owner
	mutex_id++;
	return 0;
}

int  gtthread_mutex_lock(gtthread_mutex_t *mutex)
{
	//if(mutex_array[*mutex].lock!=0 || mutex_array[*mutex].lock!=1 )   // this is wrong
	//{
	//	printf("Mutex not initialized\n");
	//	return -1;
	//}
	while(mutex_array[*mutex].lock==1)
		gtthread_yield();
	signal_block();
	mutex_array[*mutex].owner = current->id;
	mutex_array[*mutex].lock = 1;
	signal_unblock();
	return 0;
}

int  gtthread_mutex_unlock(gtthread_mutex_t *mutex)
{
	//add code to check for initialization
	//if(mutex_array[*mutex].lock!=0 || mutex_array[*mutex].lock!=1 )   // this is wrong
	//{
	//		printf("Mutex not initialized\n");
	//	return -1;
	//}
	if(current->id != mutex_array[*mutex].owner)
	{
		printf("Error in unlocking mutex\n");
		return -1;
	}
	signal_block();
	mutex_array[*mutex].lock = 0;
	//mutex_array[*mutex].owner = -1;
	signal_unblock();
	return 0;
}

void signal_block(){
	sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

void signal_unblock(){
	sigprocmask(SIG_UNBLOCK, &signal_set, NULL);	
}

void swap_thread()
{
	signal_block();
	//printf("count is %d\n", count);
	//if(t_kill == 0 && (count > 1 || (count ==1 && (current->id == current->next->id))))
	//{	
//		printf("current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
		timer_start(period_t);
		struct gtthread *prev = current;
		current = current->next;
		signal_unblock();
		swapcontext(&prev->context,&current->context);
		
	//}
	/*else
	{
//		printf("Current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
		struct gtthread *prev = current;
		current = current->next;
		//printf("current is %ld\n",current->context.uc_link);
		
//		printf("in else\n");
		t_kill = 0;
		timer_start(period_t);
		signal_unblock();
//		printf("still in else\n");
		//if(count ==1 && (current->id == current->next->id))
		//	swapcontext(&current->context,&current->context);
		//else		
			swapcontext(&ctxt_dummy,&current->context);
		
	}*/
	return;
	
}


void timer_start(long period){
	long gt_reset_time=period;
	sigemptyset(&signal_set);
	sigaddset(&signal_set,SIGVTALRM);
	timer.it_value.tv_sec=-0;
	timer.it_value.tv_usec=period;
	timer.it_interval.tv_sec=0;
	timer.it_interval.tv_usec=period;
	signal(SIGVTALRM,swap_thread);
	setitimer(ITIMER_VIRTUAL,&timer,NULL);
}


void add_thread(struct gtthread *thread)
{
	signal_block();
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
	signal_unblock();
}

void delete_thread(struct gtthread *thread)
{
	signal_block();
//	printf("thread to be deleted: %ld\n", thread->id);
	struct gtthread *temp = head;
	struct gtthread *temp_del; //to delete
	if(thread == head)
	{
		//printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
		temp_del = head;
		temp_del->parent->num_children--;
		head = head->next;
		tail->next = head;
		//free(temp_del);
	}
	while(temp->next!= head)
	{
		if(temp->next->id == thread->id) //delete next thread
		{
			temp_del = temp->next;
			if(temp_del == tail)
				tail = temp;
			temp->next = temp->next->next;
			temp_del->parent->num_children--;
			//current = temp;
		}
		else
			temp = temp->next;

	}
	count--;
	t_kill = 1; // flag to tell that current thread has been killed
	signal_unblock();
}


void check_initialization()
{
	if(period_t <=0)
	{
		printf("Initialization not done. Exiting\n");
		exit(1);
	}
}


