#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

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
	gtthread_mutex_t owner;
};

/*global declarations*/
//struct gtthread thread_array = (gtthread*)malloc(MAX_THREADS*sizeof(struct gtthread));  // array of threads
struct gtthread thread_array[MAX_THREADS];
struct gtthread_mutex mutex_array[MAX_THREADS];

/*linked list pointers*/
gtthread_t t1,t2,t3,t4,t5;
gtthread_mutex_t m1,m2,m3,m4,m5;
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
ucontext_t ctxt_main,t, dead_context;
sigset_t block;

/* function declarations */
void swap_thread();
void timer_start(long period);
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

int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);

void signal_block();
void signal_unblock();


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
	if(mutex_array[*mutex].owner<0)   // this is wrong
	{
		printf("Mutex not initialized\n");
		return -1;
	}
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
	if(current->id != mutex_array[*mutex].owner)
	{
		printf("Error in unlocking mutex\n");
		return -1;
	}
	signal_block();
	mutex_array[*mutex].lock = 0;
	mutex_array[*mutex].owner = -1;
	signal_unblock();
	return 0;
}



void signal_block(){
	sigprocmask(SIG_BLOCK, &block, NULL);
}

void signal_unblock(){
	sigprocmask(SIG_UNBLOCK, &block, NULL);	
}

void swap_thread()
{
	signal_block();
	if(t_kill == 0 && (count > 1 || (count ==1 && (current->id == current->next->id))))
	{	
		timer_start(period_t);
		struct gtthread *prev = current;
		current = current->next;
		signal_block();
		swapcontext(&prev->context,&current->context);
		
	}
	else
	{
		printf("current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
		current = current->next;
		printf("in else\n");
		t_kill = 0;
		timer_start(period_t);
		signal_block();
		swapcontext(&dead_context,&current->context);
		
	}
	return;
	
}


void timer_start(long period){
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
	printf("thread to be deleted: %ld\n", thread->id);
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
	signal_unblock();
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

void check_initialization()
{
	if(period_t <=0)
	{
		printf("Initialization not done. Exiting\n");
		exit(1);
	}
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
        new_t->context.uc_link = 0; // why???
	sigemptyset(&new_t->context.uc_sigmask);
	new_t->context.uc_stack.ss_flags=0;
	makecontext(&new_t->context, gtthread_retarg,2,start_routine,arg);
	add_thread(new_t);
	
}

int  gtthread_join(gtthread_t thread, void **status)
{
	check_initialization();
	long int temp_id = thread;	
	struct gtthread *temp	 = &thread_array[temp_id];
	
	while(temp->terminated!=1)
		gtthread_yield();
	*status = temp->retval;
	return 0;
}

void gtthread_yield()
{
	check_initialization();
	raise(SIGVTALRM);
}

void gtthread_exit(void *retval)
{
	check_initialization();
	current->terminated = 1;	
	current->retval = retval;
	if(current == main_t)
	{
		while(current->num_children>0)
		{
			gtthread_yield();
		}
	
			exit(0);
	}
	delete_thread(current);
	t_kill = 1; // flag to tell that current thread has been killed
	raise(SIGVTALRM);
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

int hello3(int a)
{
	//keep_going = 0;
	printf("passed argument is %d\n",a);
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
	gtthread_join(t1,&thread_return);
	gtthread_create(&t2, (void*(*)(void*))hello2, (void*)1000);
	gtthread_create(&t3, (void*(*)(void*))hello3, (void*)25);
	gtthread_cancel(t2);
	printf("id is %ld\n",current->id );	
	
	gtthread_join(t3,&thread_return);
	printf("returned value from 3 is %d\n", (int)thread_return);	
	gtthread_create(&t4, (void*(*)(void*))hello4, (void*)1000);
	int i;
	for(i = 0; i < 10;i++)
	{
		printf("in main:%d\n", i);
	}
	
	
	printf("hello\n");
	
	
	gtthread_join(t2,&thread_return);
	printf("returned value from 2 is %s\n", (void*)thread_return);
	gtthread_join(t4,&thread_return);
	printf("returned value from 4 is %d\n", (int)thread_return);
	printf("bla\n");
	gtthread_exit(NULL);
	return;

}
/*
void *philosopher(void *arg){
	short i;
	gtthread_mutex_t f1,f2;
	switch((int)arg){
		case 1: f1=m5; f2=m1;break;
		case 2: f1=m2; f2=m1;break;
		case 3: f1=m3; f2=m2;break;
		case 4: f1=m4; f2=m3;break;
		case 5: f1=m5; f2=m4;break;
	}
	while(1){
		printf("Philosopher #%d tries to acquire fork %lu\n",(int)arg,f1);
		gtthread_mutex_lock(&f1);
		printf("Philosopher #%d tries to acquire fork %lu\n",(int)arg,f2);
		gtthread_mutex_lock(&f2);
		printf("Philosopher #%d eating with fork %lu and %lu\n",(int)arg,f1,f2);
		gtthread_yield();
		i=rand()%9999;
		while(--i>0);
		printf("Philosopher #%d releasing fork %lu\n",(int)arg,f2);
		gtthread_mutex_unlock(&f2);
		printf("Philosopher #%d releasing fork %lu\n",(int)arg,f1);
		gtthread_mutex_unlock(&f1);
		printf("Philosopher #%d thinking\n",(int)arg);
		gtthread_yield();
		i=rand()%9999;
		while(--i>0);
	}
}

int main(){

	gtthread_init(10000);

	gtthread_mutex_init(&m1);
	gtthread_mutex_init(&m2);
	gtthread_mutex_init(&m3);
	gtthread_mutex_init(&m4);
	gtthread_mutex_init(&m5);
	gtthread_create(&t1, philosopher, (void*)1);
	gtthread_create(&t2, philosopher, (void*)2);
	gtthread_create(&t3, philosopher, (void*)3);
	gtthread_create(&t4, philosopher, (void*)4);
	gtthread_create(&t5, philosopher, (void*)5);

	while(1);
*/

