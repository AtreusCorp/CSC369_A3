#include "disk_routines.h"

unsigned char *disk;



int main(int argc, char **argv){
	unsigned int new_dir_inode_num;
	unsigned int parent_dir_inode_num;
	struct ext2_inode *parent_dir_inode;
	char target_dir[DIR_STR_LEN];
	char *parent_dir_end;
	int fd = open(argv[1], O_RDWR);

    if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    if ((new_dir_inode_num = get_inode_num(argv[2], EXT2_ROOT_INO)) > -1){
   		printf("Error: Directory already exists.");
   		return EEXIST;
  	}

    // Refers to the target directory prefixed by '/'
    parent_dir_end = strrchr(argv[2], '/');
    strncpy(target_dir, parent_dir_end + 1, DIR_STR_LEN);
    *parent_dir_end = '\0';

    if ((parent_dir_inode_num = get_inode_num(argv[2], 2)) < 0){
    	printf("Error: %s does not exist.", argv[2]);
    	return ENOENT;
    }

    parent_dir_inode = fetch_inode_from_num(parent_dir_inode_num);
    new_dir_inode_num = allocate_inode();

}