#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

#define MAX_THREADS 1000

typedef long int gtthread_t;

struct gtthread 
{
	long int id;
	struct gtthread* next;  //pointer to next node
	struct gtthread* parent;
	int terminated;
	ucontext_t context;	
	long int num_children;
	void *retval;

};

/*global declarations*/
//struct gtthread thread_array = (gtthread*)malloc(MAX_THREADS*sizeof(struct gtthread));  // array of threads
struct gtthread thread_array[MAX_THREADS];

/*linked list pointers*/
struct gtthread *head; // points to head of linked list
struct gtthread *tail; // points to tail of linked list
struct gtthread *current; //points to currently executing thread

int count=0;  //number of threads in list (active threads)
struct gtthread *main_t; // pointer to the main thread, active throughout the program
long period_t = 0; // time slice of threads, initially set to 0
long gt_id = 0; // counter to assign id's to threads

int t_kill;  //flag

struct itimerval timer;
ucontext_t ctxt_main,t1, dead_context;
sigset_t block;

/* function declarations */
void swap_thread();
void init_timer(long period);
void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);

int  gtthread_join(gtthread_t thread, void **status);

void gtthread_exit(void *retval);
void gtthread_yield(void);
gtthread_t gtthread_self(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);


void block_sig(){
	//sigprocmask(SIG_BLOCK, &block, NULL);
}

void unblock_sig(){
	//sigprocmask(SIG_UNBLOCK, &block, NULL);	
}

void swap_thread()
{
	block_sig();
	if(t_kill == 0 && (count > 1 || (count ==1 && (current->id == current->next->id))))
	{	
		init_timer(period_t);
		struct gtthread *prev = current;
		current = current->next;
		unblock_sig();
		swapcontext(&prev->context,&current->context);
		
	}
	else
	{
		current = current->next;
		printf("in else\n");
		t_kill = 0;
		init_timer(period_t);
		unblock_sig();
		swapcontext(&dead_context,&current->context);
		
	}
	return;
	
}


void init_timer(long period){
	long gt_reset_time=period;
	sigemptyset(&block);
	sigaddset(&block,SIGVTALRM);
	timer.it_value.tv_sec=-0;
	timer.it_value.tv_usec=period;
	timer.it_interval.tv_sec=0;
	timer.it_interval.tv_usec=period;
	signal(SIGVTALRM,swap_thread);
	setitimer(ITIMER_VIRTUAL,&timer,NULL);
}


void add_thread(struct gtthread *thread)
{
	block_sig();
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
	unblock_sig();
}

void delete_thread(struct gtthread *thread)
{
	block_sig();
	struct gtthread *temp = head;
	struct gtthread *temp_del; //to delete
	if(thread == head)
	{
		temp_del = head;
		temp_del->parent->num_children--;
		head = head->next;
		tail->next = head;
		free(temp_del);
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
			current = temp;
		}
		else
			temp = temp->next;

	}
	count--;
	unblock_sig();
}


void hello()
{
	//keep_going = 0;
	int i,j=0;
	for(i =0;i<100000;i++)
	{
		j++;	
	}
	printf("woohoo\n");
	return;
}
void gtthread_init(long period)
{
	period_t = period;
	main_t = &thread_array[0];
	main_t->id=gt_id;
	main_t->next= NULL;
	main_t->parent = main_t;
	main_t->num_children = 0;
	gt_id++;
	current = main_t;
	t_kill = 0;
	
	add_thread(main_t);
	init_timer(period_t);	
}

void gtthread_retarg(void *(*start_routine)(void *),void *arg)
{
	void *retval = start_routine(arg);
	gtthread_exit(retval);
}

int gtthread_create(gtthread_t *thread, void *(*start_routine)(void *),void *arg)
{
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
        new_t->context.uc_link = 0; // why???
	new_t->context.uc_stack.ss_flags=0;
	makecontext(&new_t->context, gtthread_retarg,2,start_routine,arg);
	add_thread(new_t);
	
}

int  gtthread_join(gtthread_t thread, void **status)
{
	long int temp_id = thread;	
	struct gtthread *temp	 = &thread_array[temp_id];
	
	while(temp->terminated!=1)
		gtthread_yield();
	*status = temp->retval;
	return 0;
}

void gtthread_yield()
{
	raise(SIGVTALRM);
}

void gtthread_exit(void *retval)
{
	current->terminated = 1;	
	current->retval = retval;
	while(current->num_children>0)
	{
		gtthread_yield();
	}
	if(current == main_t)
		exit(0);
	delete_thread(current);
	t_kill = 1; // flag to tell that current thread has been killed
	raise(SIGVTALRM);
}

gtthread_t gtthread_self(void)
{
	return current->id;
}

int  gtthread_equal(gtthread_t t1, gtthread_t t2)
{
	if(t1 == t2)
		return 1;
	else
		return 0;
}

int gtthread_cancel(gtthread_t thread)
{
	if(thread == current->id)
		gtthread_exit((void*)"GTTHREAD_CANCELED");
	else
	{
		struct gtthread *temp = &thread_array[thread];
		while(temp->num_children>0)
			gtthread_yield();
		temp->terminated = 1;
		temp->ret_val = (void*)"GTTHREAD_CANCELED";
		delete_thread(temp);
	}
	return 0;
}



int hello2()
{
	//keep_going = 0;
	int i,j=0;
	for(i =0;i<10000;i++)
	{
		j++;
	}
	//while(1);
	printf("in hello2\n");
	return 2;
}

int hello3()
{
	//keep_going = 0;
	int i,j=0;
	for(i =0;i<10000;i++)
	{
		j++;
	}
	//while(1);
	printf("in hello2\n");
	return 3;
}

int hello4()
{
	//keep_going = 0;
	int i,j=0;
	for(i =0;i<10000;i++)
	{
		j++;
	}
	//while(1);
	printf("in hello2\n");
	printf("I am in thread %ld\n",gtthread_self());
	return 4;
}

void main()
{
	gtthread_t t1, t2,t3,t4;
	void *thread_return = NULL;
	gtthread_init(100000);
	gtthread_create(&t1,hello, (void*)1000);
	gtthread_create(&t2, (void*(*)(void*))hello2, (void*)1000);
	gtthread_create(&t3, (void*(*)(void*))hello3, (void*)1000);
	printf("id is %ld\n",current->id );	
	
	gtthread_join(t3,&thread_return);
	printf("returned value from 3 is %d\n", (int)thread_return);	

	int i;
	for(i = 0; i < 10;i++)
	{
		printf("in main:%d\n", i);
	}
	//swapcontext(&ctxt_main,&thread_array[1].context);
	gtthread_create(&t4, (void*(*)(void*))hello4, (void*)1000);
	gtthread_join(t4,&thread_return);
	printf("returned value from 4 is %d\n", (int)thread_return);
	gtthread_join(t1,&thread_return);
	gtthread_join(t2,&thread_return);
	printf("returned value from 2 is %d\n", (int)thread_return);
	
	printf("bla\n");
	gtthread_exit(NULL);
	return;

}

