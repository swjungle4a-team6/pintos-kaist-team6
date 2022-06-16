#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "threads/synch.h"

#ifdef VM
#include "vm/vm.h"
#endif

/* States in a thread's life cycle. */
enum thread_status
{
	THREAD_RUNNING, /* Running thread. */
	THREAD_READY,	/* Not running but ready to run. */
	THREAD_BLOCKED, /* Waiting for an event to trigger. */
	THREAD_DYING	/* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t)-1) /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0	   /* Lowest priority. */
#define PRI_DEFAULT 31 /* Default priority. */
#define PRI_MAX 63	   /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct thread
{
	/* Owned by thread.c. */
	tid_t tid;				   /* Thread identifier. */
	enum thread_status status; /* Thread state. */
	char name[16];			   /* Name (for debugging purposes). */
	int priority;			   /* Priority. */
	int64_t wakeup_tick;	   // 일어날 시간

	int init_priority;		   /* donation 이후 우선순위를 초기화하기 위해 초기값 저장 */
	struct lock *wait_on_lock; /* 해당 스레드가 대기하고있는 lock자료구조의 주소를 저장 */
	struct list donations;	   /* 해당 스레드가 우선순위는 낮으나 lock을 보유하고 있을 때 사용됨 */
	struct list_elem d_elem;   /* 낮은 우선순위를 가진 스레드의 donations가 가리키는 list_elem  */
	/* Shared between thread.c and synch.c. */
	struct list_elem elem;	  /* List element. */
	struct list_elem allelem; /* advanced scheduling */

	int nice;		/* 우선순위에 영향을 주는 값 */
	int recent_cpu; /* 최근에 얼마나 많은 CPU time을 사용했는가를 표현 */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4; /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
	uintptr_t rsp;
#endif
	/* Owned by thread.c. */
	struct intr_frame tf; /* Information for switching */
	unsigned magic;		  /* Detects stack overflow. */

	/* 자식 프로세스 순회용 리스트 */
	struct list child_list;
	struct list_elem child_elem;

	/* wait_sema 를 이용하여 자식 프로세스가 종료할때까지 대기함. 종료 상태를 저장 */
	struct semaphore wait_sema;
	int exit_status;

	/* 자식에게 넘겨줄 intr_frame
	fork가 완료될때 까지 부모가 기다리게 하는 forksema
	자식 프로세스 종료상태를 부모가 받을때까지 종료를 대기하게 하는 free_sema */
	struct intr_frame parent_if;
	struct semaphore fork_sema;
	struct semaphore free_sema;

	/* fd table 파일 구조체와 fd index
	 * 각 프로세스는 자신의 File Descriptor 테이블을 가지고 있음 (파일 객체 포인터의 배열)
	 */
	struct file **fdTable; // 배열로 구현?
	int fdIdx;

	// stdin_count와 stdout_count?
	// 표준입출력인 fd가 여러 개인 경우를 고려하여 만든 변수.
	// 즉, 입출력 fd가 0이 될 때까지 close를 닫으면 안 된다.
	int stdin_count;
	int stdout_count;

	/* 현재 실행 중인 파일 */
	struct file *running;
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init(void);
void thread_start(void);

void thread_tick(void);
void thread_print_stats(void);

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *, void *);

void thread_block(void);
void thread_unblock(struct thread *);

struct thread *thread_current(void);
tid_t thread_tid(void);
const char *thread_name(void);

void thread_exit(void) NO_RETURN;
void thread_yield(void);

int thread_get_priority(void);
void thread_set_priority(int);

int thread_get_nice(void);
void thread_set_nice(int);
int thread_get_recent_cpu(void);
int thread_get_load_avg(void);

void test_max_priority(void);
bool cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

void donate_priority(void);
void remove_with_lock(struct lock *lock);
void refresh_priority(void);

void mlfqs_priority(struct thread *t);
void mlfqs_recent_cpu(struct thread *t);
void mlfqs_load_avg(void);
void mlfqs_increment(void);
void mlfqs_recalc_priority(void);
void mlfqs_recalc_recent_cpu(void);
int thread_get_recent_cpu(void);

void do_iret(struct intr_frame *tf);

#define FDT_PAGES 3
#define FDCOUNT_LIMIT FDT_PAGES *(1 << 9)

#endif /* threads/thread.h */
