/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

#define SECTORS_PER_PAGE 8

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
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
	//PGSIZE(4096bytes) = 8 * disk_sectors(512bytes), 페이지 하나당 나타낼bit 하나만 있으면 됨
	swap_table = bitmap_create(disk_size(swap_disk) / 8); //sector 8개당 bit 하나
	//bitmap_set_all(swap_table, 0); //create에서 해줌
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	/* 누구 코드더라.. 승희와 킹관 */
	page->operations = &anon_ops;

	// enum vm_type type_ = page_get_type(page);
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));
	struct anon_page *anon_page = &page->anon;
	anon_page->swap_slot = 0;
	
	// anon_page->swap_slot = -1;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	// printf("	anon_swap_in 들어왔다!!!%p\n", anon_swap_in);
	struct anon_page *anon_page = &page->anon;
	int swap_idx = anon_page->swap_slot;
	//void *kva = page->frame->kva;
	// printf("hihihihi~~~~3\n");
	for (int i = 0; i < 8; i++)
	{
		disk_read(swap_disk, swap_idx*SECTORS_PER_PAGE + i, kva + i*DISK_SECTOR_SIZE);

	}
	//disk_read(swap_disk, anon_page->swap_slot, page->frame->kva);
	// printf("hihihihi~~~~2\n");
	// bitmap_set(swap_table, (anon_page->swap_slot)/8, 0);
	bitmap_set(swap_table, swap_idx, 0);
	// printf("hihihihi~~~~1\n");
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	// printf("	### anon_swap_out ###\n");
	//printf("	anon_swap_out 들어왔달룽\n");
	struct anon_page *anon_page = &page->anon;
	struct thread *t = thread_current();
	// if (bitmap_all(swap_table, 0, disk_size(swap_disk) / 8))
	// {
	// 	PANIC("swap table is full.");
	// }
	// printf("	### bitmap_all 통과 ###\n");
	
	int swap_idx = (int)bitmap_scan_and_flip(swap_table, 0, 1, false); //start 0, count 1, find 0->change to 1
	anon_page->swap_slot = (int)swap_idx;
	// printf("	### bitmap_scan_and_flip 통과 ###\n");

	for (int i =0; i < SECTORS_PER_PAGE; i++)
	{
		disk_write(swap_disk, swap_idx*SECTORS_PER_PAGE + i, page->va + i*DISK_SECTOR_SIZE);//왜 page->frame->kva아님?
	}
	// printf("	### for문 통과: idx: %d ###\n", swap_idx);
	//pml4_set_dirty(t->pml4, page->va, 0);
	pml4_clear_page(t->pml4, page->va); //그냥 present bit을 0으로 해놔야
	palloc_free_page(page->frame->kva); //process_cleanup()에서 palloc_free_page안하고 살려둠
	list_remove(&page->frame->f_elem);
	//list_remove(&page->frame->f_elem); //get_victim에서 해줌
	//page->frame->page = NULL;

	/*ㅇㅁㅇ spt에서 지우면 이건 영원히 추방...*/
	//spt_delete_page(&t->spt, page); //struct page에 대해서만 지워줌.(이 struct가 담고 있는 정보에 해당하는 애도 지워줘야함)
	return true;

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	// struct anon_page *anon_page = &page->anon;
	// struct frame *frame = page->frame;

	// // printf("	at anon_destroy(), will remove and free frame\n");
	// if (frame != NULL) {
	// 	// palloc_free_page(frame->kva);
	// 	list_remove(&frame->f_elem);
	// 	free(frame);
	// }
	// printf("	at anon_destroy(), just removed frame from list\n");
}
