#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "init.h"
#include "lib/user/syscall.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);
void check_address(uintptr_t *addr);
void get_argument(uintptr_t *rsp, int *arg, int count);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface
 * 시스템 콜 번호를 이용하여 해당 시스템 콜의 서비스 루틴을 호출 하도록 구현
 * 유저 스택 포인터(rsp) 주소와 시스템 콜 인자가 가리키는 주소(포인터)가 유효 주소 ((유저 영역)인지 확인하도록 구현
 * 기존 핀토스는 유저영역을 벗어난 주소를 참조할 경우 page fault 발생
 * 유저 스택에 존재하는 스택 프레임의 인자들을 커널에 복사하도록 구현
 * 시스템 콜의 함수의 리턴 값은 intr_frame의 rax에 저장되도록 구현
 */
void syscall_handler(struct intr_frame *f UNUSED)
{
	check_address(f->rsp);

	// TODO: Your implementation goes here.
	printf("system call!\n");
	// thread_exit();

	switch (f->R.rax)
	{

	/* Projects 2 and later. */
	/* Halt the operating system. */
	case SYS_HALT:
		power_off();
		break;

	/* Terminate this process. */
	case SYS_EXIT:
		exit(NULL);
		break;

	/* Clone current process. */
	case SYS_FORK:
		break;

	/* Switch current process. */
	case SYS_EXEC:
		break;

	/* Wait for a child process to die. */
	case SYS_WAIT:
		break;

	/* Create a file. */
	case SYS_CREATE:
		break;

	/* Delete a file. */
	case SYS_REMOVE:
		break;

	/* Open a file. */
	case SYS_OPEN:
		break;

	/* Obtain a file's size. */
	case SYS_FILESIZE:
		break;

	/* Read from a file. */
	case SYS_READ:
		break;

	/* Write to a file. */
	case SYS_WRITE:
		break;

	/* Change position in a file. */
	case SYS_SEEK:
		break;

	/* Report current position in a file. */
	case SYS_TELL:
		break;

	/* Close a file. */
	case SYS_CLOSE:
		break;

	/* Extra for Project 2
	 * Duplicate the file descriptor to kernel area
	 */
	case SYS_DUP2:
		break;
	case SYS_MOUNT:
		break;
	case SYS_UMOUNT:
		break;
	}
}

/* check_address()
 * 주소 값이 유저 영역에서 사용하는 주소 값((0x8048000~0xc0000000))인지 확인 하는 함수
 * 유저 영역을 벗어난 경우 프로세스 종료 (exit(-1))
 */
void check_address(uintptr_t *addr)
{
	/* TODO : User Memory Access */
}

/* get_argument()
 * 유저 스택에 있는 인자들을 커널에 저장하는 함수
 * 스택 포인터(_if->rsp)에 count(인자의 개수) 만큼의 데이터를 arg에 저장
 * int *arg (스택 메모리가 아닌 커널 영역)
 */
void get_argument(uintptr_t *rsp, int *arg, int count)
{
}