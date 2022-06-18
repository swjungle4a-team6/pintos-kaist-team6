#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {
	struct segment *file_aux;
	// off_t page_ofs;
	// int page_count; //mmap시 읽어야 할 length 저장할 용도
	// struct file *target_file;
	// size_t written_bytes;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
bool lazy_load_file(struct page *page, void *aux);
#endif
