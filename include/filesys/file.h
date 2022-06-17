#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H
#include <stdbool.h>
#include "filesys/off_t.h"

struct inode;

struct file
{
    int ref_cnt;
    struct inode *inode; /* File's inode. */
    off_t pos;           /* Current position. */
    bool deny_write;     /* Has file_deny_write() been called? */
    // 표준 입출력을 가리키는 fd가 아닌 경우도 있으니 dup_count변수를 만들어야 한다.
    int dupCount; /* dupCount 가 0일때만 close() */
};

/* Opening and closing files. */
struct file *file_open(struct inode *);
struct file *file_reopen(struct file *);
struct file *file_duplicate(struct file *file);
void file_close(struct file *);
struct inode *file_get_inode(struct file *);

/* Reading and writing. */
off_t file_read(struct file *, void *, off_t);
off_t file_read_at(struct file *, void *, off_t size, off_t start);
off_t file_write(struct file *, const void *, off_t);
off_t file_write_at(struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write(struct file *);
void file_allow_write(struct file *);

/* File position. */
void file_seek(struct file *, off_t);
off_t file_tell(struct file *);
off_t file_length(struct file *);

#endif /* filesys/file.h */