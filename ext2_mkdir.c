#include "disk_routines.h"

unsigned char *disk;



int main(int argc, char **argv){
	unsigned int new_dir_inode_num;
	int parent_dir_inode_num;
	struct ext2_inode *parent_dir_inode;
	char target_dir[EXT2_NAME_LEN];
    char parent_dir[EXT2_NAME_LEN];
	char *parent_dir_end;
	int fd = open(argv[1], O_RDWR);

    if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    if ((new_dir_inode_num = get_inode_num(argv[2], EXT2_ROOT_INO)) > -1){
   		printf("Error: Directory already exists.\n");
   		return EEXIST;
  	}

    // Refers to the target directory prefixed by '/'
    parent_dir_end = strrchr(argv[2], '/');

    // If the target directory had a slash at the end, handle accordingly
    if (strlen(parent_dir_end + 1) == 0){
        *parent_dir_end = '\0';
        parent_dir_end = strrchr(argv[2], '/');
    }

    // Extract the target and parent directory names
    strncpy(target_dir, parent_dir_end + 1, EXT2_NAME_LEN);
    *(parent_dir_end + 1) = '\0';
    strncpy(parent_dir, argv[2], EXT2_NAME_LEN);

    if ((parent_dir_inode_num = get_inode_num(parent_dir, 2)) < 0){
    	printf("Error: %s does not exist.\n", parent_dir);
    	return ENOENT;
    }
    parent_dir_inode = fetch_inode_from_num(parent_dir_inode_num);
    
    if (insert_dir_entry(parent_dir_inode, parent_dir_inode_num, 0, strlen(target_dir), EXT2_FT_DIR, target_dir) < 0){
    	exit(1);
    }
    return 0;
}