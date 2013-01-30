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
int t_kill;

struct itimerval timer;
ucontext_t ctxt_main,t1, dead_context;
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

void block_sig(){
	//sigprocmask(SIG_BLOCK, &block, NULL);
}

void unblock_sig(){
	//sigprocmask(SIG_UNBLOCK, &block, NULL);	
}

void swap_thread()
{
	//if(ll->kill==0 && (ll->count>1 || (ll->count==1 && ll->curr->gt_id==ll->curr->next->gt_id))){
	printf("in swap here\n");
	printf("current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
	//printf("count is %d\n", count);
	block_sig();
	if(t_kill == 0 && (count > 1 || (count ==1 && (current->id == current->next->id))))
	{	
		init_timer(period_t);
	//	printf("current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
		//printf("current is %ld, next is %ld\n",current->context.uc_stack.ss_size, current->next->context.uc_stack.ss_size );
		struct gtthread_t *prev = current;
		current = current->next;
	//	printf("Current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
		unblock_sig();
		//if(!current->id == current->next->id)
		//{ 
		//	printf("in if\n");
			
			swapcontext(&prev->context,&current->context);
		//}
		//swapcontext(&prev->context,&prev->context);
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
	//printf("timer is %ld\n", period);
	signal(SIGVTALRM,swap_thread);
	setitimer(ITIMER_VIRTUAL,&timer,NULL);
	//sleep(10);
}


void add_thread(struct gtthread_t *thread)
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

void delete_thread(struct gtthread_t *thread)
{
	block_sig();
	struct gtthread_t *temp = head;
	struct gtthread_t *temp_del; //to delete
	//printf("here\n");
	//check if *thread is head or tail
	if(thread == head)
	{
		temp_del = head;
		temp_del->parent->num_children--;
		head = head->next;
		tail->next = head;
		free(temp_del);
	}
	//printf("thread to be deleted: %ld\n", thread->id);
	//printf("in delete:current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
	while(temp->next!= head)
	{
		if(temp->next->id == thread->id) //delete next thread
		{
			temp_del = temp->next;
			//printf("here3\n");
			if(temp_del == tail)
				tail = temp;
			temp->next = temp->next->next;
			//printf("here4\n");
			temp_del->parent->num_children--;
			current = temp;
			//free(temp_del);
		}
		else
			temp = temp->next;

	}
	count--;
	//printf("after delete:current is %ld, next is %ld, next next is %ld\n",current->id, current->next->id, current->next->next->id );
	unblock_sig();
}


void hello()
{
	keep_going = 0;
	int i,j=0;
	while(1);
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
	main_t = &gtarray[0];
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
	//printf("in retarg in thread %d return value is %d\n", current->id, (int)retval);
	gtthread_exit(retval);
}

int gtthread_create(struct gtthread_t *thread, void *(*start_routine)(void *),void *arg)
{
	struct gtthread_t *new_t = &gtarray[gt_id];
	new_t->id = gt_id;
	thread->id = gt_id;
	//printf("gt id is %d\n",gt_id);
	gt_id++;
	
	new_t->next = NULL;
	new_t->terminated= 0;
	new_t->parent = current;
	new_t->num_children = 0;
	current->num_children++;
	thread = new_t;
	//char stack1[16384];
	getcontext(&new_t->context);
	new_t->context.uc_stack.ss_sp = malloc(16384);
        new_t->context.uc_stack.ss_size = 16384;
        new_t->context.uc_link = 0; // why???
	new_t->context.uc_stack.ss_flags=0;
	makecontext(&new_t->context, gtthread_retarg,2,start_routine,arg);
	add_thread(new_t);
	
}

int  gtthread_join(struct gtthread_t thread, void **status)
{
	long int temp_id = thread.id;	
	//printf("temp id is %ld\n", temp_id);
	struct gtthread_t *temp	 = &gtarray[temp_id];
	//printf("here2\n");
	
	while(temp->terminated!=1)
		gtthread_yield();
	//printf("here in %ld\n", current->id);
	*status = temp->retval;
	//printf("in join return thread %ld return is %d\n", temp->id,temp->retval);
	return 0;
}

void gtthread_yield()
{
	//printf("yielding\n");
	raise(SIGVTALRM);
}

void gtthread_exit(void *retval)
{
	//printf("boo from %ld\n");	
	current->terminated = 1;	
	current->retval = retval;
	//printf("in exit return value of %ld is %d\n", current->id, current->retval);
	while(current->num_children>0)
	{
		gtthread_yield();
	}
	if(current == main_t)
		exit(0);
	delete_thread(current);
	t_kill = 1;
	raise(SIGVTALRM);

}



int hello2()
{
	//keep_going = 0;
	int i,j=0;
	for(i =0;i<10000;i++)
	{
		j++;
	}
	while(1);
	printf("in hello2\n");
	return 2;
}

void main()
{
	struct gtthread_t t1, t2,t3,t4;
	void *thread_return = NULL;
	gtthread_init(100000);
	gtthread_create(&t1, hello, (void*)1000);
	gtthread_create(&t2, hello2, (void*)1000);
	gtthread_create(&t3, hello, (void*)1000);
	printf("id is %d\n",current->id );	
	
	gtthread_join(t2,&thread_return);
	printf("returned value is %d\n", (int)thread_return);
	gtthread_join(t1,&thread_return);
	gtthread_join(t3,&thread_return);
		

	int i;
	for(i = 0; i < 10;i++)
	{
		printf("in main:%d\n", i);
	}
	//swapcontext(&ctxt_main,&gtarray[1].context);
	gtthread_create(&t4, hello, (void*)1000);
	gtthread_join(t4,&thread_return);

	printf("bla\n");
	gtthread_exit(NULL);
	return;

}

