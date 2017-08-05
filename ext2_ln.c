#include "disk_routines.h"

unsigned char *disk;

int main(int argc, char **argv){
	int soft_flag = 0;
	int targeted_file_index = 2;
	int targeted_file_path_len;
	int target_dir_inode_num;
	struct ext2_inode *target_dir_inode;
    //TODO: Is it correct to assume dir_name's length is less EXT2_NAME_LEN + 1
	char target_dir_name[EXT2_NAME_LEN + 1];
	char target_file_name[EXT2_NAME_LEN + 1];
	char *target_dir_name_end = NULL;
	int source_file_inode_num;
	int source_file_path_len;
	struct ext2_inode *source_file_inode;
	int source_file_index = 3;
	int fd = open(argv[1], O_RDWR);


	if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    // Check for the "soft" flag
	if (strncmp(argv[2], "-s", 2) == 0){
		soft_flag = 1;
		targeted_file_index = 3;
		source_file_index = 4;
	}

	// Checks
	targeted_file_path_len = strlen(argv[targeted_file_index]);
	source_file_path_len = strlen(argv[source_file_index]);

	// check if inode for source_file is valid 
	if((source_file_inode_num = get_inode_num(argv[source_file_index], EXT2_ROOT_INO)) < 0) {
		printf("Error: The source file does not exist.\n");
		return ENOENT;
	}
	source_file_inode = fetch_inode_from_num(source_file_inode_num);

	// Check if either is directory 
	if (argv[targeted_file_index][targeted_file_path_len - 1] == '/'
		|| argv[source_file_index][source_file_path_len - 1] == '/'
		|| source_file_inode->i_mode & EXT2_S_IFDIR){

		printf("Error: Directories are not allowed.\n");
		return EISDIR;
	}

	// Check if targeted file exists already
	if (get_inode_num(argv[targeted_file_index], EXT2_ROOT_INO) >= 0){
		
		printf("Error: Target file already exists.\n");
		return EEXIST;
	}

	if ((target_dir_name_end = strrchr(argv[targeted_file_index], '/')) == NULL){
        printf("Error: %s does not exist.\n", argv[2]);
        return ENOENT;
    }

    strncpy(target_file_name, target_dir_name_end + 1, EXT2_NAME_LEN);
    *(target_dir_name_end + 1) = '\0';
    strncpy(target_dir_name, argv[targeted_file_index], EXT2_NAME_LEN);

	if((target_dir_inode_num = get_inode_num(target_dir_name, EXT2_ROOT_INO)) < 0) {
		
		printf("Error: The target path does not exist.\n");
		return ENOENT;
	}
	target_dir_inode = fetch_inode_from_num(target_dir_inode_num);


	if (soft_flag == 0) {
		if (insert_dir_entry(target_dir_inode, 
		// The case for hard link
		target_dir_inode_num, 
		source_file_inode_num,
		strlen(target_file_name),
		EXT2_FT_REG_FILE,
		target_file_name
		) < 0) {
			return -1;
		}

		++source_file_inode->i_links_count;
		return 0;
	} else {
		// The case for soft link 
		// TODO: Should we store the first slash?
		unsigned char *beginning_of_block;
		int new_file_inode_num;
		struct ext2_inode *new_file_inode;

		if (insert_dir_entry(target_dir_inode, 
		target_dir_inode_num, 
		0,
		strlen(target_file_name),
		EXT2_FT_SYMLINK,
		target_file_name
		) < 0) {

			return -1;
		}

		if((new_file_inode_num = search_dir(target_file_name, target_dir_inode)) < 0){

			return -1;
		}

		new_file_inode = fetch_inode_from_num(new_file_inode_num);
		beginning_of_block = disk + new_file_inode->i_block[0] * EXT2_BLOCK_SIZE;
		strncpy((char *) beginning_of_block, argv[source_file_index] + 1, EXT2_NAME_LEN);
		return 0;
	}
}