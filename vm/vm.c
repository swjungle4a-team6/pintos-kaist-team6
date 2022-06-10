/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "include/threads/vaddr.h"

struct list *frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
	list_init(frame_table);
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check whether the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
// Find struct page that corresponds to va from the given supplemental page table. If fail, return NULL.

struct page *spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = (struct page *)malloc(sizeof(struct page));
	/* TODO: Fill this function. */

	/* pg_round_down()로 offset을 제외한 vaddr의 페이지 번호를 얻음*/
	page->va = pg_round_down(va);

	/* hash_find() 함수를 사용해서 hash_elem 구조체 얻음 */
	struct hash_elem *h = hash_find(spt->hash, &page->h_elem);

	/* 만약 존재하지 않는다면 NULL리턴*/
	if (h == NULL)
		return NULL;

	free(page);

	/* hash_entry()로 해당 hash_elem의 vm_entry 구조체 리턴*/
	return hash_entry(h, struct page, h_elem);
}

/* Insert PAGE into spt with validation. */
// Insert 'struct page' into the given supplemental page table. This function should checks that the virtual address does not exist in the given supplemental page table.
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	/* TODO: Fill this function. */
	if (hash_insert(&spt->hash, &page->h_elem))
		return false;

	return true;
}

/* Delete PAGE from spt */
bool spt_delete_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	/* TODO: Fill this function. */
	if (hash_delete(&spt->hash, &page->h_elem))
		return false;

	return true;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/

/* palloc_get_page를 호출하여 사용자 풀에서 새 실제 페이지를 가져옵니다.
 * Gets a new physical page from the user pool by calling palloc_get_page. When successfully got a page from the user pool, also allocates a frame, initialize its members, and returns it. After you implement vm_get_frame, you have to allocate all user space pages (PALLOC_USER) through this function. You don't need to handle swap out for now in case of page allocation failure. Just mark those case with PANIC ("todo") for now.

 * 사용자 풀에서 페이지를 성공적으로 가져오면 프레임도 할당하고 구성원을 초기화한 다음 반환합니다.
 * vm_get_frame을 구현한 후에는 이 기능을 통해 모든 사용자 공간 페이지(PALLOC_USER)를 할당해야 합니다. 페이지 할당에 실패하는 경우 지금은 스왑 아웃을 처리할 필요가 없습니다.
 * 일단 이 경우 패닉("TO DO")으로 표시하십시오. */
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	/* TODO: Fill this function. */
	ASSERT(frame != NULL);

	frame->kva = palloc_get_page(PAL_USER);

	if (frame->kva)
	{
		frame->page = NULL;
	}

	else
	{
		PANIC("TO DO");
	}

	ASSERT(frame->page == NULL);

	/* ------------------------- */
	return frame;
}

/* Claim the page that allocate on VA. */
/* va를 할당하도록 페이지를 클레임합니다.
 * 먼저 페이지를 받은 후 vm_do_claim_page를 호출해야 합니다.*/
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = spt_find_page(&thread_current()->spt, va);
	/* TODO: Fill this function */
	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
/* vm_get_frame을 호출하여 프레임을 얻습니다(템플릿에서 이미 수행됨).
 * 가상 주소에서 페이지 테이블의 실제 주소로 매핑을 추가
 * 반환 값은 작업의 성공 여부를 나타내야 합니다 (True or False).
 */
static bool
vm_do_claim_page(struct page *page)
{
	printf("	page: %#x\n", page);
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert pte(page table entry) to map page's VA to frame's PA. */
	list_push_back(frame_table, &frame->f_elem);
	/* ------------------------------------------------------------------ */

	return swap_in(page, frame->kva);
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// page = spt_find_page(spt, addr);
	// if (!page)
	// {
	// 	return false;
	// }

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	/* hash_destroy()으로 해시테이블의 버킷리스트와 vm_entry들을 제거 ? */
	// hash_destroy(&spt->hash, NULL);
}

/* Returns a hash value for page p. */
unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry(p_, struct page, h_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, h_elem);
	const struct page *b = hash_entry(b_, struct page, h_elem);

	return a->va < b->va;
}