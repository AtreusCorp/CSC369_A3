CC=gcc
CFLAGS=-Wall -g
DEPS=disk_routines.h ext2.h subroutines.c
BASE=subroutines.c
BINARIES=ext2_ls ext2_mkdir ext2_ln ext2_rm ext2_rm_bonus

all: $(SRCS) $(DEPS)
		$(CC) $(CFLAGS) $(BASE) ext2_ls.c -o ext2_ls
		$(CC) $(CFLAGS) $(BASE) ext2_mkdir.c -o ext2_mkdir
		$(CC) $(CFLAGS) $(BASE) ext2_ln.c -o ext2_ln
		$(CC) $(CFLAGS) $(BASE) ext2_rm.c -o ext2_rm
		$(CC) $(CFLAGS) $(BASE) ext2_rm_bonus.c -o ext2_rm_bonus

%: %.c $(DEPS)
		$(CC) $(CFLAGS) $(BASE) $< -o $<.c
clean:
		rm -f $(BINARIES)