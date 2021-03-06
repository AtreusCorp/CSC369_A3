#include "disk_routines.h"

unsigned char *disk;

void print_entries(struct ext2_inode *dir_inode, int a_flag){
    struct ext2_super_block *super_block = 
        (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
	int i;
	unsigned char *first_entry;
    unsigned char *cur_entry;
    char *cur_dir = ".";
    char *parent_dir = "..";

	for(i = 0; i < 12; ++i){
        if (check_block_bitmap(dir_inode->i_block[i])
            && dir_inode->i_block[i] >= super_block->s_first_data_block){

            // Keep track of beginning to ensure block overflow doesn't occur
            first_entry = disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE;
            cur_entry = first_entry;

            do {
                struct ext2_dir_entry_2 *dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);
                
                // TODO: Ask: can we still assume 1 is the bad inode?
                if (dir_entry->inode >= 1){

                	const char *dir_entry_name = dir_entry->name;
                	const size_t name_len = (size_t) dir_entry->name_len;

                	// If the all flag is present or the directory is not . or ..
					if (a_flag || !((strncmp(dir_entry_name, cur_dir, name_len) == 0)
                			   	    || (strncmp(dir_entry_name, parent_dir, name_len) == 0))){

						printf("%.*s\n", (int) name_len, dir_entry_name);
                	}
                }

                // Skip to the next entry
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
	int dir_inode_num;
	int indirect_dir_inode_num;
	int fd = open(argv[1], O_RDWR);

    if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    // Check for the "all" flag
	if (strncmp(argv[2], "-a", 2) == 0){
		a_flag = 1;
		path_index = 3;
	}

	if ((dir_inode_num = get_inode_num(argv[path_index], EXT2_ROOT_INO)) == -1){
		printf("No such file or directory\n");
		return ENOENT;
	}

	dir_inode = fetch_inode_from_num(dir_inode_num);
	

	// Handle the case where ls is called on a file instead of a dir
	if (S_ISDIR(dir_inode->i_mode)){
		print_entries(dir_inode, a_flag);

		// First level of inode indirection
		indirect_dir_inode_num = dir_inode->i_block[12];

		if (indirect_dir_inode_num >= EXT2_GOOD_OLD_FIRST_INO 
			&& check_inode_bitmap(indirect_dir_inode_num)){
			print_entries(fetch_inode_from_num(indirect_dir_inode_num), a_flag);
		}

	} else if (S_ISREG(dir_inode->i_mode) || S_ISLNK(dir_inode->i_mode)){
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