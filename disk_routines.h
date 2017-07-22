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

#define DIR_STR_LEN 255


extern unsigned char *disk;

extern struct ext2_inode *fetch_inode_from_num(unsigned int);
extern unsigned int search_dir(char *, struct ext2_inode *);
extern unsigned int get_inode_num(char *, unsigned int);

#endif