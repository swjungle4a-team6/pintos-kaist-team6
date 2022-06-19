#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "lib/kernel/bitmap.h"
struct page;
enum vm_type;

struct anon_page
{
    int swap_slot;
    bool is_stack;
};

struct bitmap *swap_table;

void vm_anon_init(void);
bool anon_initializer(struct page *page, enum vm_type type, void *kva);

#endif
