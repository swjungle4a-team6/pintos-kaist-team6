#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "include/threads/synch.h"
#include "include/lib/kernel/bitmap.h"

struct page;
enum vm_type;

struct anon_page
{
    uint64_t swap_slot_index;
    // bool is_stack;
};

struct swap_table
{
    struct lock swap_lock;
    struct bitmap bitmap;
};

void vm_anon_init(void);
bool anon_initializer(struct page *page, enum vm_type type, void *kva);

#endif
