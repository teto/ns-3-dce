/* Heavily inspired by posix-host.c with some calls
 * prefixed with `dce_` */
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <lkl_host.h>
/* #include <linux/sched.h> */
/* #include <linux/wait.h> */


// for dce-signal.h
typedef void (*sighandler_t)(int);

// todo rename folder to include
#include <model/dce-stdio.h>
#include <model/dce-stdlib.h> /* for dce_malloc/free */
#include <model/dce-pthread.h> /* for pthread */
#include <model/dce-signal.h> /* for panic */
#include <model/dce-time.h> /* for panic */
#include <model/dce-timerfd.h> /* for timerfr_XX */


/* for now rely on mutex since dce has everything available
 * _POSIX_SEMAPHORES
 * we could even move some of the sem
 * 
 * */
#undef _POSIX_SEMAPHORES

struct lkl_sem {
#ifdef _POSIX_SEMAPHORES
	sem_t sem;
#else
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
#endif /* _POSIX_SEMAPHORES */
};

/* lkl_thread_t
 * typedef unsigned long lkl_thread_t;
 * */
struct lkl_tls_key {
	pthread_key_t key;
};

struct lkl_mutex {
	pthread_mutex_t mutex;
};

#define WARN_UNLESS(exp) do {						\
		if (exp < 0)						\
			lkl_printf("%s: %s\n", #exp, strerror(errno));	\
	} while (0)

/* struct SimTask { */
/* 	struct task_struct kernel_task; */
/* 	void *private; */
/* }; */

  #define IMPORT_FROM_LKL(name

/* extern struct SimTask *sim_task_create(void *private, unsigned long pid); */
/* sim_init */
/* struct SimKernel * */
/* see setup.c */
/* void sim_init (m_exported, &imported, (struct SimKernel *)this) { */

/*   } */

static int _warn_pthread(int ret, char *str_exp)
{
	if (ret > 0)
		lkl_printf("%s: %s\n", str_exp, strerror(ret));

	return ret;
}

/* pthread_* functions use the reverse convention */
#define WARN_PTHREAD(exp) _warn_pthread(exp, #exp)

static lkl_thread_t thread_create(void (*fn)(void *), void *arg)
{
	pthread_t thread;
	if (WARN_PTHREAD(dce_pthread_create(&thread, NULL, (void* (*)(void *))fn, arg)))
		return 0;
	else
		return (lkl_thread_t) thread;
}

static void thread_detach(void)
{
	WARN_PTHREAD(pthread_detach(pthread_self()));
}

static int thread_join(lkl_thread_t tid)
{
	if (WARN_PTHREAD(dce_pthread_join((pthread_t)tid, NULL)))
		return -1;
	else
		return 0;
}

static void thread_exit(void)
{
	dce_pthread_exit(NULL);
}

static int thread_equal(lkl_thread_t a, lkl_thread_t b)
{
	return dce_pthread_equal(a, b);
}


/* in dce utils.cc and dce-time.cc through functions like 
 * UtilsTimeToTimespec/
 *
 * g_timeBase can set the initial time
 *  */
static unsigned long long time_ns(void)
{
	struct timespec ts;

	/* DCE ignores CLOCK_MONOTONIC */
	dce_clock_gettime(CLOCK_MONOTONIC, &ts);

	return 1e9*ts.tv_sec + ts.tv_nsec;
}


// TODO use host/node malloc
static struct lkl_sem *sem_alloc(int count)
{
	struct lkl_sem *sem;

	sem = malloc(sizeof(*sem));
	if (!sem)
		return NULL;

#ifdef _POSIX_SEMAPHORES
	if (sem_init(&sem->sem, SHARE_SEM, count) < 0) {
		lkl_printf("sem_init: %s\n", strerror(errno));
		free(sem);
		return NULL;
	}
#else
	pthread_mutex_init(&sem->lock, NULL);
	sem->count = count;
	WARN_PTHREAD(pthread_cond_init(&sem->cond, NULL));
#endif /* _POSIX_SEMAPHORES */

	return sem;
}

static void sem_free(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_destroy(&sem->sem));
#else
	WARN_PTHREAD(pthread_cond_destroy(&sem->cond));
	WARN_PTHREAD(pthread_mutex_destroy(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
	free(sem);
}

static void sem_up(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_post(&sem->sem));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	sem->count++;
	if (sem->count > 0)
		WARN_PTHREAD(pthread_cond_signal(&sem->cond));
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */

}

static void sem_down(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	int err;

	do {
		err = sem_wait(&sem->sem);
	} while (err < 0 && errno == EINTR);
	if (err < 0 && errno != EINTR)
		lkl_printf("sem_wait: %s\n", strerror(errno));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	while (sem->count <= 0)
		WARN_PTHREAD(pthread_cond_wait(&sem->cond, &sem->lock));
	sem->count--;
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
}

static struct lkl_mutex *mutex_alloc(int recursive)
{
	struct lkl_mutex *_mutex = malloc(sizeof(struct lkl_mutex));
	pthread_mutex_t *mutex = NULL;
	pthread_mutexattr_t attr;

	if (!_mutex)
		return NULL;

	mutex = &_mutex->mutex;
	WARN_PTHREAD(dce_pthread_mutexattr_init(&attr));

	/* PTHREAD_MUTEX_ERRORCHECK is *very* useful for debugging,
	 * but has some overhead, so we provide an option to turn it
	 * off. */
#ifdef DEBUG
	if (!recursive)
		WARN_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
#endif /* DEBUG */

	if (recursive)
		WARN_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));

	WARN_PTHREAD(pthread_mutex_init(mutex, &attr));

	return _mutex;
}

static void mutex_lock(struct lkl_mutex *mutex)
{
	WARN_PTHREAD(pthread_mutex_lock(&mutex->mutex));
}

static void mutex_unlock(struct lkl_mutex *_mutex)
{
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_unlock(mutex));
}

static void mutex_free(struct lkl_mutex *_mutex)
{
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_destroy(mutex));
	free(_mutex);
}


static struct lkl_tls_key* tls_alloc(void (*destructor)(void *))
{
	// NOP
	return NULL;
}

static void tls_free(struct lkl_tls_key *key)
{
	// NOP
}

static int tls_set(struct lkl_tls_key *key, void *data)
{
	// NOP
	return 0;
}

static void* tls_get(struct lkl_tls_key *key)
{
	// NOP
	return NULL;
}

//static void* mem_alloc(unsigned long count)
//{
//	// NOP
//	return sim_malloc(count);
//}

//static void mem_free(void *addr)
//{
//	// NOP
//}

/* maybe we can do better with dce */
static void *timer_alloc(void (*fn)(void *), void *arg)
{
	int err;
	timer_t timer;
	struct sigevent se =  {
		.sigev_notify = SIGEV_THREAD,
		.sigev_value = {
			.sival_ptr = arg,
		},
		.sigev_notify_function = (void (*)(union sigval))fn,
	};

	err = dce_timerfd_create(CLOCK_REALTIME, &se, &timer);
	if (err)
		return NULL;

	return (void *)(long)timer;
}

static int timer_set_oneshot(void *_timer, unsigned long ns)
{
	timer_t timer = (timer_t)(long)_timer;
	struct itimerspec ts = {
		.it_value = {
			.tv_sec = ns / 1000000000,
			.tv_nsec = ns % 1000000000,
		},
	};

	return dce_timerfd_settime(timer, 0, &ts, NULL);
}

static void timer_free(void *_timer)
{
	timer_t timer = (timer_t)(long)_timer;

	timer_delete(timer);
}

static void* lkl_ioremap(long addr, int size)
{
	// NOP
}

static int lkl_iomem_access(const __volatile__ void *addr, void *val, int size,
		int write)
{
	// NOP
}

static long gettid(void)
{
	// returns a pid_t,
	return dce_gettid();
}

/* static void jmp_buf_set(struct lkl_jmp_buf *jmpb, void (*f)(void)) */
/* { */
/* 	// NOP */
/* } */

/* static void jmp_buf_longjmp(struct lkl_jmp_buf *jmpb, int val) */
/* { */
/* 	// NOP */
/* } */
static lkl_thread_t thread_self(void)
{
	return (lkl_thread_t)dce_pthread_self();
}

static void print(const char *str, int len)
{
	/* int ret __attribute__((unused)); */
	/* ret = write(STDOUT_FILENO, str, len); */
	dce_printf(str);
}


/* look at posix host for some understanding */
struct lkl_host_operations lkl_host_ops = {
	.print = print,
	/* for now a paste of dce_abort */
	.panic = dce_panic,

	/* most of these are available already in DCE
	 * we just use wrappers to fix the return type */
	.thread_create = thread_create,
	.thread_detach = thread_detach,
	.thread_exit = thread_exit,  /* returns void */
	.thread_join = thread_join,
	.thread_self = thread_self,
	.thread_equal = thread_equal,

	/* using posix version */
	.sem_alloc = sem_alloc,
	.sem_free = sem_free,
	.sem_up = sem_up,
	.sem_down = sem_down,

	/* posix inspired */
	.mutex_alloc = mutex_alloc,
	.mutex_free = mutex_free,
	.mutex_lock = mutex_lock,
	.mutex_unlock = mutex_unlock,

	/* todo */
	.tls_alloc = tls_alloc,
	.tls_free = tls_free,
	.tls_set = tls_set,
	.tls_get = tls_get,

	.time = time_ns,
	.timer_alloc = timer_alloc,
	.timer_set_oneshot = timer_set_oneshot,
	.timer_free = timer_free,

	.mem_alloc = dce_malloc,
	.mem_free = dce_free,

	/* TODO */
	.ioremap = lkl_ioremap,
	.iomem_access = lkl_iomem_access,
	.virtio_devices = lkl_virtio_devs,

  /* @gettid - returns the host thread id of the caller, which need not */
  /* be the same as the handle returned by thread_create */
	.gettid = gettid,

	/* pthread_fiber_manager does some things
	 * maybe try to reestablish the single codepath */
	.jmp_buf_set = 0,
	.jmp_buf_longjmp = 0,  /* jmp_buf_longjmp */
};
