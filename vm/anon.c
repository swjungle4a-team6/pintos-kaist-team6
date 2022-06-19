/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "include/threads/mmu.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
struct bitmap *swap_table;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	swap_table = bitmap_create(disk_size(swap_disk) / 8);
	// printf("	### swap_disk : %d,	swap_table : %d	###		", swap_disk, swap_table);
	// printf("	### disk_size: %d	###", disk_size(swap_disk));
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	/* 누구 코드더라.. 승희와 킹관 */
	page->operations = &anon_ops;

	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	struct anon_page *anon_page = &page->anon;
	anon_page->swap_slot_index = 0;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	// printf(" 들어오라ㅗ############################################\n");
	struct anon_page *anon_page = &page->anon;

	// index 가져와야 함
	int page_num = anon_page->swap_slot_index;

	for (int i = 0; i < SECTORS_PER_PAGE; i++)
	{
		disk_read(swap_disk, (page_num * SECTORS_PER_PAGE) + i, kva + DISK_SECTOR_SIZE * i);
		// disk, disk_sector_t, buffer;
	}

	bitmap_set(swap_table, page_num, false);

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	struct anon_page *anon_page = &page->anon;

	int slot_index = bitmap_scan_and_flip(swap_table, 0, 1, false);

	for (int i = 0; i < SECTORS_PER_PAGE; i++)
	{
		disk_write(swap_disk, (slot_index * SECTORS_PER_PAGE) + i, page->va + DISK_SECTOR_SIZE * i);
	}

	anon_page->swap_slot_index = slot_index;
	pml4_clear_page(thread_current()->pml4, page->va);
	palloc_free_page(page->frame->kva); // by. 혜느님

	list_remove(&page->frame->f_elem);

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	struct frame *frame = page->frame;

	// list_remove(&frame->f_elem);
	// free(frame);
}
