#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "ext2.h"
#ifndef ROUTINES
#define ROUTINES

#define DIR_STR_LEN 255


extern unsigned char *disk;
extern struct ext2_group_desc *group_desc;
extern struct ext2_inode *inode_table;

#endif