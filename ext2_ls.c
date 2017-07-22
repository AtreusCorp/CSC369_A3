#include "disk_routines.h"

unsigned char *disk;

void print_entries(struct ext2_inode *dir_inode, int a_flag){
	int i;
	unsigned char *first_entry;
    unsigned char *cur_entry;
    char *cur_dir = ".";
    char *parent_dir = "..";
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            	(disk + 2 * EXT2_BLOCK_SIZE);

	for(i = 0; i < 12; ++i){
        if (((unsigned int) pow(2, dir_inode->i_block[i])) | group_desc->bg_block_bitmap){
            first_entry = disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE;
            cur_entry = first_entry;

            do {
                struct ext2_dir_entry_2 *dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);
                
                // TODO: Ask: can we still assume 1 is the bad inode?
                if (dir_entry->inode >= 1){

                	const char *dir_entry_name = dir_entry->name;
                	const size_t name_len = (size_t) dir_entry->name_len;

					if (a_flag || !((strncmp(dir_entry_name, cur_dir, name_len) == 0)
                			   	    || (strncmp(dir_entry_name, parent_dir, name_len) == 0))){

						printf("%.*s\n", (int) name_len, dir_entry_name);
                	}
                }
                cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;
            } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
        } else {
            break;
        }
    }
}

int main(int argc, char **argv){
	int a_flag = 0;
	int path_index = 2;
	struct ext2_inode *dir_inode;
	unsigned int dir_inode_num;
	unsigned int indirect_dir_inode_num;
	unsigned int indirect_num_one_hot;
	struct ext2_group_desc *group_desc;

	int fd = open(argv[1], O_RDWR);

    if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    group_desc = (struct ext2_group_desc *) (disk + 2 * EXT2_BLOCK_SIZE);

	if (strncmp(argv[2], "-a", 2) == 0){
		a_flag = 1;
		path_index = 3;
	}

	if ((dir_inode_num = get_inode_num(argv[path_index], EXT2_ROOT_INO)) == -1){
		printf("No such file or directory\n");
		return ENOENT;
	}

	dir_inode = fetch_inode_from_num(dir_inode_num);
	
	if (S_ISDIR(dir_inode->i_mode)){
		print_entries(dir_inode, a_flag);

		// Indirect inode
		indirect_dir_inode_num = dir_inode->i_block[12];

		indirect_num_one_hot = pow(2, indirect_dir_inode_num);
		if (indirect_dir_inode_num >= 1 && indirect_num_one_hot | group_desc->bg_inode_bitmap){
			print_entries(fetch_inode_from_num(indirect_dir_inode_num), a_flag);
		}
	} else if (S_ISREG(dir_inode->i_mode)){
		char *last_slash = strrchr(argv[path_index], '/');
		last_slash += 1;
		if (strlen(last_slash) == 0){

			// File was referenced as a directory
			printf("No such file or directory\n");
		} else {
			printf("%s\n", last_slash);
		}

	}
	return 0;

}