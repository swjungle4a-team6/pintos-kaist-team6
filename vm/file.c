/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
/* memory mapped file */
#include "include/userprog/syscall.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"
/* ------------------ */

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
}

/* Initialize the file backed page
 * 메모리를 지원하는 파일과 같은 페이지 구조의 일부 정보를 업데이트할 수 있음
 * * aux malloc, memcpy?
 */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	/* project3 */
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));
	/* -------------------------------------- */

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller.
 * struct page를 free할 필요 없음
 * if content is dirty, 변경 내용을 파일에 다시 써야함
 */
static void
file_backed_destroy(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
	// if (pml4_is_dirty(NULL, NULL))
	// {
	// }
}

/* Do the mmap
 * addr : 새로운 맵핑의 시작 주소 (should be aligend to page boundary)
 * * if NULL, the kernel chooses the address
 * * Otherwise, the kernel takes it as a hint about where to place the mapping
 * length : 맵핑의 길이
 * offset : file mapping을 위한 file offset
 */
void *
do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset)
{
	// if (addr == NULL || length == NULL)
	// {
	// 	return false;
	// }

	// if (!vm_alloc_page_with_initializer(VM_FILE, addr, true, lazy_load_segment, NULL))
	// {
	// 	return false;
	// }

	// struct page *p = spt_find_page(&thread_current()->spt, addr);

	// if (p == NULL)
	// {
	// 	return false;
	// }

	// p->file.file = file;
	// p->file.length = length;
	// p->file.offset = offset;
	// p->file.writable = writable;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	struct thread *t = thread_current();
	struct page *p = spt_find_page(&t->spt, addr);

	// munmap() 이전에 close()가 호출되어도 file mapping은 여전히 유효
	// file_reopen(NULL);
}
