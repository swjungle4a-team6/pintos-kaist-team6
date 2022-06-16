#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd(const char *file_name);
tid_t process_fork(const char *name, struct intr_frame *if_);
int process_exec(void *f_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(struct thread *next);
void argument_stack(char **parse, int count, void **rsp);

/* pid를 입력하여 자식프로세스인지 확인하여 맞다면 thread 구조체 반환 */
struct thread *get_child_process(int pid);

/* project 3 - virtual memory */
bool setup_stack(struct intr_frame *if_);
static bool lazy_load_segment(struct page *page, void *aux);
/* ---------------------------*/

#endif /* userprog/process.h */