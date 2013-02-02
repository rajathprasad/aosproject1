#include "gtthread.c"


extern void gtthread_init(long period);
extern int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);

extern int  gtthread_join(gtthread_t thread, void **status);

extern void gtthread_exit(void *retval);
extern void gtthread_yield(void);
extern gtthread_t gtthread_self(void);
extern int  gtthread_equal(gtthread_t t1, gtthread_t t2);
extern int  gtthread_cancel(gtthread_t thread);

extern int  gtthread_mutex_init(gtthread_mutex_t *mutex);
extern int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
extern int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);


