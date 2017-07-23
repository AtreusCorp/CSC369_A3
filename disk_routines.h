#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include "ext2.h"
#ifndef ROUTINES
#define ROUTINES


extern unsigned char *disk;

extern struct ext2_inode *fetch_inode_from_num(unsigned int);
extern int search_dir(char *, struct ext2_inode *);
extern int get_inode_num(char *, unsigned int);
extern int allocate_inode();
extern int insert_dir_entry(struct ext2_inode *, 
							unsigned int, 
                          	int,
                            unsigned char,
                            unsigned char,
                            char *);

#endif