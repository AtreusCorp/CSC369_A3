#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "ext2.h"
#ifndef ROUTINES
#define ROUTINES


extern unsigned char *disk;

extern struct ext2_inode *fetch_inode_from_num(unsigned int);
extern int search_dir(char *, struct ext2_inode *);
extern int get_inode_num(char *, unsigned int);
extern int allocate_inode();
extern int allocate_block();
extern int check_inode_bitmap(int);
extern void set_inode_bitmap(int);
extern void unset_inode_bitmap(int);
extern int check_block_bitmap(int);
extern void set_block_bitmap(int);
extern void unset_block_bitmap(int);
extern int insert_dir_entry(struct ext2_inode *, 
							unsigned int, 
                          	int,
                            unsigned char,
                            unsigned char,
                            char *);
extern int remove_dir_entry(struct ext2_inode *, 
                     		char *,
                     		unsigned int);
extern void assign_block_inode(struct ext2_inode *pinode,
							   unsigned int inode_blk_num,
							   unsigned int blk_num);

#endif