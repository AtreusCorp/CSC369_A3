#include "disk_routines.h"

unsigned char *disk;


/* Unsets all bitmap flags used by the given inode.
 */
void unset_blocks_and_inodes(unsigned int inode_num, 
                             struct ext2_inode *inode){

    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unset_inode_bitmap(inode_num);
    super_block->s_free_inodes_count += 1;
    group_desc->bg_free_inodes_count += 1;

    for (int i = 0; i < 12; ++i){
        if (inode->i_block[i] >= EXT2_GOOD_OLD_FIRST_INO){
            unset_block_bitmap(inode->i_block[i]);
            super_block->s_free_blocks_count += 1;
            group_desc->bg_free_blocks_count += 1;
        }
    }

    if (inode->i_block[12] >= super_block->s_first_data_block){
        int *indirect_inode = (int *)(disk + inode->i_block[12] * EXT2_BLOCK_SIZE);
        unset_inode_bitmap(*indirect_inode);
        super_block->s_free_inodes_count += 1;
        group_desc->bg_free_inodes_count += 1;

        for (int i = 0; i < EXT2_BLOCK_SIZE / sizeof(int); ++i){
            if (*(indirect_inode + i) >= super_block->s_first_data_block){
                unset_block_bitmap(*(indirect_inode + i));
                super_block->s_free_blocks_count += 1;
                group_desc->bg_free_blocks_count += 1;
            } else {
                break;
            }
        }
    }
}

void recursive_remove(unsigned int dir_inum_to_delete, 
                      unsigned int parent_inum){
    int i;
    struct ext2_super_block *super_block = 
        (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    unsigned char *first_entry;
    unsigned char *cur_entry;
    struct ext2_inode *dir_inode_to_delete = 
        fetch_inode_from_num(dir_inum_to_delete);

    if (S_ISDIR(dir_inode_to_delete->i_mode) != 1){

        dir_inode_to_delete->i_dtime = time(NULL);
        --dir_inode_to_delete->i_links_count;
        if (dir_inode_to_delete->i_links_count == 0){
            unset_blocks_and_inodes(dir_inum_to_delete, 
                                    dir_inode_to_delete);
        }
        return;
    }
    for(i = 0; i < 12; ++i){

        if (!(check_block_bitmap(dir_inode_to_delete->i_block[i]))
            || dir_inode_to_delete->i_block[i] <= super_block->s_first_data_block){
            
            break;
        }
        first_entry = disk + dir_inode_to_delete->i_block[i] * EXT2_BLOCK_SIZE;
        cur_entry = first_entry;

        do {
            struct ext2_dir_entry_2 *dir_entry = ((struct ext2_dir_entry_2 *) 
                                                    cur_entry);
            unsigned int dir_entry_inode_num = dir_entry->inode;
            unsigned int dir_entry_rec_len = dir_entry->rec_len;
            struct ext2_inode *dir_entry_inode = 
                                    fetch_inode_from_num(dir_entry_inode_num);
            
            if (dir_entry_inode_num >= EXT2_GOOD_OLD_FIRST_INO
                && dir_entry_inode_num != dir_inum_to_delete
                && dir_entry_inode_num != parent_inum){

                if (S_ISDIR(dir_entry_inode->i_mode)){

                    recursive_remove(dir_entry_inode_num, 
                                     dir_inum_to_delete);
                    --dir_entry_inode->i_links_count;

                    if (dir_entry_inode->i_links_count == 0){
                        unset_blocks_and_inodes(dir_entry_inode_num, 
                                                dir_entry_inode);
                    }
                } else {
                    char dir_entry_name[EXT2_NAME_LEN + 1];
                    strncpy(dir_entry_name, dir_entry->name, dir_entry->name_len);
                    dir_entry_name[dir_entry->name_len] = '\0';
                    remove_dir_entry(dir_inode_to_delete, dir_entry_name, 
                                     dir_entry_inode_num);
                    --dir_entry_inode->i_links_count;

                    if (dir_entry_inode->i_links_count == 0){
                        unset_blocks_and_inodes(dir_entry_inode_num, 
                                                dir_entry_inode);
                    }
                }
            } else {
                if (dir_entry_inode_num == dir_inum_to_delete){
                    --dir_inode_to_delete->i_links_count;
                } else if (dir_entry_inode_num == parent_inum){
                    --fetch_inode_from_num(parent_inum)->i_links_count;
                }

            }
            cur_entry += dir_entry_rec_len;
        } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
    }
    dir_inode_to_delete->i_dtime = time(NULL);
    --dir_inode_to_delete->i_links_count;
    if (dir_inode_to_delete->i_links_count == 0){
        unset_blocks_and_inodes(dir_inum_to_delete, 
                                dir_inode_to_delete);
    }
    return;
}

int main(int argc, char **argv){
    int r_flag = 0;
    int path_index = 2;
	int target_inode_num;
	int parent_dir_inode_num;
	struct ext2_inode *parent_dir_inode;
	struct ext2_inode *target_inode;
	char target_dir[EXT2_NAME_LEN + 1];
    char parent_dir[EXT2_NAME_LEN + 1];
	char *parent_dir_end;
	int fd = open(argv[1], O_RDWR);

    if (strncmp(argv[2], "-r", 2) == 0){
        r_flag = 1;
        path_index = 3;
    }

    if((disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
    	perror("Error");
    	exit(1);
    }

    if ((target_inode_num = get_inode_num(argv[path_index], EXT2_ROOT_INO)) < 0){
   		printf("Error: File does not exist.\n");
   		return EEXIST;
  	}
  	target_inode = fetch_inode_from_num(target_inode_num);

  	if (!(target_inode->i_mode & EXT2_S_IFREG)
        && !r_flag){
  		printf("Error: The given file is a directory. Use the -r flag\n");
   		return -1;
  	}

    // Refers to the target directory prefixed by '/'
    parent_dir_end = strrchr(argv[path_index], '/');

    // If the target directory had a slash at the end, handle accordingly
    if (strlen(parent_dir_end + 1) == 0){
        *parent_dir_end = '\0';
        parent_dir_end = strrchr(argv[path_index], '/');
    }

    // Extract the target and parent directory names
    if (strlen(parent_dir_end + 1) > EXT2_NAME_LEN){
        printf("Error: File name too long, "
        	   "file cannot exist on this system.\n");
        return EEXIST;
    }

    strncpy(target_dir, parent_dir_end + 1, EXT2_NAME_LEN);
    *(parent_dir_end + 1) = '\0';
    strncpy(parent_dir, argv[path_index], EXT2_NAME_LEN);

    if ((parent_dir_inode_num = get_inode_num(parent_dir, 2)) < 0){
    	printf("Error: %s does not exist.\n", parent_dir);
    	return ENOENT;
    }
    parent_dir_inode = fetch_inode_from_num(parent_dir_inode_num);

    if(remove_dir_entry(parent_dir_inode, target_dir, target_inode_num) < 0){
        return -1;
    }
    
    recursive_remove(target_inode_num, parent_dir_inode_num);

    return 0;
}