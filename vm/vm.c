/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "include/threads/mmu.h"
#include "include/threads/vaddr.h"
#include "include/userprog/process.h"
#include "vm/file.h"

struct list frame_table;
struct list_elem *clock_pointer;
static void vm_stack_growth(void *rsp, void *addr UNUSED);
/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_table);
	//clock_pointer = list_head(&frame_table);
	/* -------------------------- */
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
 * page, do not create it directly and make it through this function or `vm_alloc_page`.
 * 커널이 새 페이지 요청을 수신할 때 호출
 */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{
	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = NULL;
	/* Check whether the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		// 전달받은 aux에 따라 INIT을 호출?
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		page = (struct page *)malloc(sizeof(struct page));
		if (!page) return false;
		bool (*initializer)(struct page *, enum vm_type, void *);

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			initializer = &anon_initializer;
			break;

		case VM_FILE:
			initializer = &file_backed_initializer;
			break;
		default:
			goto err;
		}

		upage = pg_round_down(upage);
		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable;

		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, page) ? true : false;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
// Find struct page that corresponds to va from the given supplemental page table. If fail, return NULL.

struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function. */

	struct page tmp;
	struct hash_elem *h = NULL;
	tmp.va = pg_round_down(va);

	h = hash_find(&spt->hash, &tmp.h_elem);

	if (h == NULL)
	{
		return NULL;
	}

	return hash_entry(h, struct page, h_elem);
}
/* Insert PAGE into spt with validation. */
// Insert 'struct page' into the given supplemental page table. This function should checks that the virtual address does not exist in the given supplemental page table.
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	/* TODO: Fill this function. */
	page->va = pg_round_down(page->va);
	if (!hash_insert(&spt->hash, &page->h_elem)) // hash_insert는 삽입 성공 시 0을 반환 (헷갈리면 안됨)
		return true;

	return false;
}

/* Delete PAGE from spt */
bool spt_delete_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	/* TODO: Fill this function. */
	//printf("	entered spt_delete_page\n");
	hash_delete(&spt->hash, &page->h_elem);
	vm_dealloc_page(page);
	return true;
}

// void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
// {
// 	vm_dealloc_page(page);
// 	return true;
// }

/* Get the struct frame, that will be evicted. */
static struct frame *vm_get_victim(void)
{
	struct frame *victim = NULL;
	
	if (!list_empty(&frame_table))
	{
		victim = list_entry(list_pop_back(&frame_table), struct frame, f_elem);
	}
	return victim;
}

/* 3차 윤찬이네 알고리즘 */
// static struct frame *
// vm_get_victim(void)
// {
// 	// printf("\nvm_get_victim() entry\n");

// 	// lock_acquire(&frame_lock);
// 	struct frame *victim = NULL;
// 	/* TODO: The policy for eviction is up to you. */
// 	// clock 알고리즘. 
	
// 	struct list_elem *elem;
// 	struct frame *frame;

// 	for (elem = list_begin(&frame_table); elem != list_end(&frame_table);
// 		 elem = list_next(elem)){
		
// 		frame = list_entry(elem, struct frame, f_elem);
		
// 		bool access = pml4_is_accessed(thread_current()->pml4, frame->page->va);

// 		if (!access){
// 			victim = frame;
// 			list_remove(&frame->f_elem);
// 			break;
// 		}else{
// 			pml4_set_accessed(thread_current()->pml4, frame->page->va, false);
// 		}
		
// 	}
// 	if (victim == NULL){
// 		victim = list_entry(list_pop_front(&frame_table), struct frame, f_elem);
		
// 	}
// 	// lock_release(&frame_lock);
// 	// printf("\nvm_get_victim() end %p\n", victim);
// 	return victim;
// }

/* 2차? 내가 짠 알고리즘 */
// static struct frame *vm_get_victim(void)
// {
// 	struct frame *victim = NULL;
// 	/* TODO: The policy for eviction is up to you. */
// 	//struct list_elem *clock_pointer = list_head(&frame_table);
// 	uint64_t *pml4 = &thread_current()->pml4;
// 	struct list_elem *tmp_elem;

// 	for (int i = 0; i<2; i++)
// 	{
// 		// printf("frame: %p\n", list_front(&frame_table));
// 		for (tmp_elem = list_front(&frame_table); tmp_elem != list_end(&frame_table); tmp_elem = list_next(tmp_elem))
// 		{
// 			victim = list_entry(tmp_elem, struct frame, f_elem);
// 			if (pml4_is_accessed(pml4, victim->page->va))
// 			{
// 				pml4_set_accessed(pml4, victim->page->va, 0);
// 			}
// 			else
// 			{
// 				if (victim != NULL) {
// 					list_remove(&victim->f_elem);
// 					return victim;
// 				}
// 			}
// 		}
// 	}
// 	return NULL;
// }
/* 1차 민우 clock 알고리즘 */
// 	struct list_elem *tmp_elem = clock_pointer;
// 	uint64_t *pml4 = &thread_current()->pml4;
// 	for (tmp_elem; tmp_elem != list_end(&frame_table); tmp_elem = list_next(tmp_elem))
// 	{
// 		victim = list_entry(tmp_elem, struct frame, f_elem);
// 		if (pml4_is_accessed(pml4, victim->page->va))
// 		{
// 			pml4_set_accessed(pml4, victim->page->va, 0);
// 		}
// 		else
// 		{
// 			clock_pointer = list_next(tmp_elem);
// 			return victim;
// 		}
// 	}

// 	for (tmp_elem = list_begin(&frame_table); tmp_elem != clock_pointer; tmp_elem = list_next(tmp_elem))
// 	{
// 		victim = list_entry(tmp_elem, struct frame, f_elem);
// 		if (pml4_is_accessed(pml4, victim->page->va))
// 		{
// 			pml4_set_accessed(pml4, victim->page->va, 0);
// 		}
// 		else
// 		{
// 			clock_pointer = list_next(tmp_elem);
// 			return victim;
// 		}
// 	}
// }

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */
	// if (!victim) return NULL;
	
	return swap_out(victim->page) ? victim : NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/

/* palloc_get_page를 호출하여 사용자 풀에서 새 실제 페이지를 가져옵니다.
 * Gets a new physical page from the user pool by calling palloc_get_page. When successfully got a page from the user pool, also allocates a frame, initialize its members, and returns it. After you implement vm_get_frame, you have to allocate all user space pages (PALLOC_USER) through this function. You don't need to handle swap out for now in case of page allocation failure. Just mark those case with PANIC ("todo") for now.

 * 사용자 풀에서 페이지를 성공적으로 가져오면 프레임도 할당하고 구성원을 초기화한 다음 반환합니다.
 * vm_get_frame을 구현한 후에는 이 기능을 통해 모든 사용자 공간 페이지(PALLOC_USER)를 할당해야 합니다. 페이지 할당에 실패하는 경우 지금은 스왑 아웃을 처리할 필요가 없습니다.
 * 일단 이 경우 PANIC("TO DO")으로 표시하십시오. */
static struct frame * // 여기서 얻은 frame을 담을 frame page table을 구현해줘야할 것 같은데?
vm_get_frame(void)
{
	/* TODO: Fill this function. */
	// printf("============start==========\n");
	struct frame *frame = NULL;
	void *temp = palloc_get_page(PAL_USER);

	if (temp == NULL)
	{
		frame = vm_evict_frame();
		if (!frame) return NULL;
		frame->page->frame = NULL;
		temp = palloc_get_page(PAL_USER); //temp = frame->page->kva;
		free(frame); //NULL값으로 돌아오는지 비워진 프레임이 있는지 확인하는 용도였어서
	}
	else
	{
	}
	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = temp;
	frame->page = NULL;
	list_push_back(&frame_table, &frame->f_elem);

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}
	// struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	// frame->kva = palloc_get_page(PAL_USER);

	// if (frame->kva == NULL)
	// {
	// 	frame = vm_evict_frame();
	// 	if (!frame)
	// 	{
	// 		return NULL;
	// 	}
	// }
	// else
	// {
	// 	list_push_back(&frame_table, &frame->f_elem);
	// 	frame->page = NULL;
		
	// 	ASSERT(frame != NULL);
	// 	ASSERT(frame->page == NULL);
	// 	return frame;
	// }


/* Claim the page that allocate on VA. */
/* va를 할당하도록 페이지를 클레임합니다.
 * 먼저 페이지를 받은 후 vm_do_claim_page를 호출해야 합니다.*/
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	struct supplemental_page_table *spt = &thread_current()->spt;
	/* TODO: Fill this function */
	page = spt_find_page(spt, va);
	/* ------------------------ */
	return page != NULL ? vm_do_claim_page(page) : false;
}

/* Claim the PAGE and set up the mmu. */
/* vm_get_frame을 호출하여 프레임을 얻습니다(템플릿에서 이미 수행됨).
 * 가상 주소에서 페이지 테이블의 실제 주소로 매핑을 추가
 * 반환 값은 작업의 성공 여부를 나타내야 합니다 (True or False).
 */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();
	struct thread *curr = thread_current();
	/* Set links */
	frame->page = page;
	page->frame = frame;

	// printf("=============end========\n");
	// printf("va: %p\n", page->va);
	// printf("kva: %p\n", frame->kva);
	// printf("page: %p\n", page);
	if (pml4_get_page(curr->pml4, page->va) == NULL && pml4_set_page(curr->pml4, page->va, frame->kva, page->writable)) {
		// printf("file: %p", file_backed_swap_in);
		// printf("init: %p\n", page->operations->swap_in);
		bool check = swap_in(page, frame->kva);
		// printf("bool: %d\n", check);
		return check;
	}

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	return false;
}

/* Growing the stack. */
static void
vm_stack_growth(void *rsp, void *addr UNUSED)
{
	//printf("	vm_stack_growth 들어옴 \n");
	if (addr >= (USER_STACK - 0x100000 + PGSIZE))
	{
		addr = pg_round_down(addr);
		//printf("	addr = %p\n", addr);
		vm_alloc_page(VM_ANON | VM_MARKER_0, addr, 1);
	}
	
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
	//printf("	vm_try_handle_fault 들어옴\n");
	// addr : page_fault가 발생한 가상주소?
	struct thread *t = thread_current();
	struct supplemental_page_table *spt UNUSED = &t->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	if (!not_present)
	{
		return false;
	}

	void *rsp = (void*)(user ? f->rsp : thread_current()->rsp);
	//void *rsp = f->rsp;
	//printf("	rsp = %p\n", rsp);
	if ((rsp <= addr && addr < USER_STACK) || rsp - addr == 0x8)
	{
		vm_stack_growth(rsp, addr);
	}
	// else{
		// printf("	failed to grow stack, rsp = %p, addr = %p\n", rsp, addr);
	// }
	
	page = spt_find_page(spt, addr);
	//printf("	page = %p\n", page);
	return page ? vm_do_claim_page(page) : false;
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
	// todo 2: This is used when a child needs to inherit the execution context of its parent (i.e. fork()).
	// todo 2: Iterate through each page in the src's supplemental page table and make a exact copy of the entry in the dst's supplemental page table.
	// todo 2: You will need to allocate uninit page and claim them immediately.
	// printf("~~~~~~hihi~~~~~~~~");
	struct hash_iterator i;
	hash_first(&i, &src->hash);
	while (hash_next(&i))
	{
		struct page *p = hash_entry(hash_cur(&i), struct page, h_elem);
		struct page *new_p;
		// 이렇게 해도 되나 ? -> ok: current thread == dst (child)
		if (p->operations->type == VM_UNINIT)
		{
			if (p->uninit.type & VM_MARKER_0) //스택일 경우
			{ // 스택
				if (!setup_stack(&thread_current()->tf))
				{
					return false;
				}
			}
			//초기페이지
			struct segment *aux = malloc(sizeof(struct segment));
			memcpy(aux, p->uninit.aux, sizeof(struct segment)); // copy aux
			if (!vm_alloc_page_with_initializer(page_get_type(p), p->va, p->writable, p->uninit.init, aux))
			{
				free(aux);
				return false;
			}
		}
		// else if (p->operations->type == VM_FILE) //이미 file-backed로 초기화된 페이지들
		// {
		// 	//printf("copying spt!!\n");
		// 	struct segment *aux = (struct segment *)malloc(sizeof(struct segment));
		// 	memcpy(aux, p->file.file_aux, sizeof(struct segment));
		// 	//memcpy(aux, p->uninit.aux, sizeof(struct segment)); // copy aux
		// 	if (!vm_alloc_page_with_initializer(page_get_type(p), p->va, p->writable, lazy_load_file, aux))
		// 	{
		// 		free(aux);
		// 		return false;
		// 	}
		// }
		else
		{
			if (!vm_alloc_page(page_get_type(p), p->va, p->writable))
			{ // page 할당
				return false;
			}
			if (!vm_claim_page(p->va))
			{ // mapping
				return false;
			}
			// printf("~~~~");
			new_p = spt_find_page(dst, p->va);
			// printf("~~~~");
			memcpy(new_p->frame->kva, p->frame->kva, PGSIZE); // 메모리 복사
			// printf("~~~~");
		}
	}
	return true;
}
// bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
// 								  struct supplemental_page_table *src UNUSED)
// {
// 	struct hash_iterator i;
// 	hash_first(&i, &src->hash);
// 	while (hash_next(&i))
// 	{
// 		struct page *parent_page = hash_entry(hash_cur(&i), struct page, h_elem);
// 		enum vm_type type = page_get_type(parent_page);
// 		void *upage = parent_page->va;
// 		bool writable = parent_page->writable;
// 		vm_initializer *init = parent_page->uninit.init; // 부모의 초기화되지 않은 페이지들 할당하기 위해
// 		void *aux = parent_page->uninit.aux;
// 		if (parent_page->uninit.type & VM_MARKER_0)
// 		{
// 			setup_stack(&thread_current()->tf);
// 		}
// 		else if (parent_page->operations->type == VM_UNINIT)
// 		{
// 			// struct page *child_page = spt_find_page(dst, upage);
// 			// memcpy(child_page->uninit.aux, parent_page->uninit.aux, sizeof(struct segment));
// 			if (!vm_alloc_page_with_initializer(type, upage, writable, init, aux))
// 				return false;
// 		}
// 		else
// 		{
// 			if (!vm_alloc_page(type, upage, writable))
// 				return false;
// 			if (!vm_claim_page(upage))
// 				return false;
// 		}

// 		if (parent_page->operations->type != VM_UNINIT)
// 		{ // 부모의 타입이 VM_UNINIT이 아닐 경우
// 			struct page *child_page = spt_find_page(dst, upage);
// 			memcpy(child_page->frame->kva, parent_page->frame->kva, PGSIZE);
// 		}
// 	}
// 	return true;
// }

void page_destructor(struct hash_elem *h, void *aux UNUSED)
{
	/* Get hash element (hash_entry() 사용) */
	struct page *p = hash_entry(h, struct page, h_elem);
	vm_dealloc_page(p);
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	hash_destroy(&spt->hash, page_destructor);
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