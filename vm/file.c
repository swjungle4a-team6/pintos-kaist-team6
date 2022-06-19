/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"
#include "include/threads/thread.h"
#include "include/filesys/file.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);

bool lazy_load_file(struct page *page, void *aux);

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

/* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	/* project3 */
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));
	/* -------------------------------------- */
	struct file_page *file_page = &page->file;

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
	// struct segment *aux = (struct segment *)page->uninit.aux;

	struct segment *aux = (struct segment *)file_page->file_aux;

	struct file *file = aux->file;
	off_t offset = aux->ofs;
	size_t page_read_bytes = aux->written_bytes;
	size_t page_zero_bytes = PGSIZE - page_read_bytes;

	if (file_read_at(file, kva, page_read_bytes, offset) != (int)page_read_bytes)
	{
		return false;
	}

	memset(kva + page_read_bytes, 0, page_zero_bytes);

	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
	struct thread *t = thread_current();
	struct segment *aux = (struct segment *)file_page->file_aux;

	if (pml4_is_dirty(t->pml4, page->va))
	{
		file_write_at(aux->file, page->va, aux->written_bytes, aux->ofs);
		pml4_set_dirty(t->pml4, page->va, 0);
	}

	list_remove(&page->frame->f_elem);
	pml4_clear_page(thread_current()->pml4, page->va);
	palloc_free_page(page->frame->kva);

	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page)
{
	struct segment *aux UNUSED = page->file.file_aux;
	struct frame *frame = page->frame;
	struct thread *t = thread_current();

	if (pml4_is_dirty(t->pml4, page->va))
	{
		file_write_at(aux->file, page->frame->kva, aux->written_bytes, aux->ofs);
		pml4_set_dirty(t->pml4, page->va, 0);
	}
	// list_remove(&frame->f_elem);
	// free(frame);
}

/* Do the mmap */
void *
do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset)
{
	if ((long)length < offset)
	{
		return NULL;
	}
	struct file *ofile = file_reopen(file);
	void *origin_addr = addr;
	if (spt_find_page(&thread_current()->spt, addr) != NULL)
	{
		return NULL;
	} //사실 이건 vm_alloc 들어가면 페이지마다 확인해주긴 함

	int page_count = (length % PGSIZE ? (int)(length / PGSIZE) + 1 : (int)(length / PGSIZE));

	//조건 통과
	long zero_bytes = 0;
	while (length > 0 || zero_bytes > 0)
	{
		// printf("	do_mmap들어옴!!\n");
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct segment *segment = malloc(sizeof(struct segment));
		segment->ofs = offset;
		segment->read_bytes = page_read_bytes;
		segment->zero_bytes = page_zero_bytes;
		segment->file = ofile;
		segment->page_count = page_count;
		void *aux = segment;

		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_file, aux))
			return NULL;

		/* Advance */
		length -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
		// printf("	do_mmap 한번 완료\n");
	}

	return origin_addr;
	// vm_alloc_page_with_initializer(VM_FILE, addr, 1, lazy_load_segment, aux);
}

bool lazy_load_file(struct page *page, void *aux)
{
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
	// printf("	lazy_load_file 들어옴\n");
	struct segment *aux_data = aux;
	struct file *f = aux_data->file;
	off_t ofs = aux_data->ofs;
	uint32_t page_read_bytes = aux_data->read_bytes;
	uint32_t page_zero_bytes = aux_data->zero_bytes;

	page->file.file_aux = (struct segment *)malloc(sizeof(struct segment));
	memcpy(page->file.file_aux, aux_data, sizeof(struct segment));

	file_seek(f, ofs);
	size_t written_bytes = file_read(f, page->frame->kva, page_read_bytes);
	if (written_bytes == NULL)
	{
		return false;
	}
	// uninit일 때와 달라지는 점은 이거 하나
	page->file.file_aux->written_bytes = written_bytes;
	// printf("written_bytes = %d\n", written_bytes);

	memset(page->frame->kva + page_read_bytes, 0, page_zero_bytes);

	free(aux_data);
	// printf("	lazy_load_file 완료\n");
	return true;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	struct thread *t = thread_current();
	struct page *page = spt_find_page(&t->spt, addr);
	if (!page)
	{
		return;
	}
	struct segment *aux = page->file.file_aux;

	while (aux->page_count--)
	{
		if (pml4_is_dirty(t->pml4, page->va))
		{
			file_write_at(aux->file, page->frame->kva, aux->written_bytes, aux->ofs);
			pml4_set_dirty(t->pml4, page->va, 0);
		}

		spt_delete_page(&t->spt, page);
		if (!(page = spt_find_page(&t->spt, addr + PGSIZE)))
			return;

		aux = page->file.file_aux;
	}

	// file_close(aux->file);
}
