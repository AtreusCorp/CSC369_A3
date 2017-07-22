#include "disk_routines.h"

// Multiplying by 2 to account for passing boot block and super block
// put these in main
struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
struct ext2_inode *inode_table = (struct ext2_inode *)(disk + group_desc->bg_inode_table 
                                                              * EXT2_BLOCK_SIZE);

/* Returns a pointer to the inode in the memory mapped into by
 * disk corresponding to inode_num.
 */
struct ext2_inode *fetch_inode(unsigned int inode_num){

    return inode_table + (inode_table * sizeof(ext2_inode));
}

int search_dir_direct_blks(char * file_name, struct ext2_inode *dir_inode) {
    int i;
    char *first_entry;
    char *cur_entry;

    if (S_ISDIR(dir_inode->i_mode) != 1){
        return -1;
    }

    for(i = 0; i < 12; ++i){
        if ((2 ** dir_inode->i_block[i]) | group_desc->bg_block_bitmap){
            first_entry = disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE;
            cur_entry = first_entry;

            do {
                struct ext2_dir_entry_2 *dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);
                const char *dir_entry_name = dir_entry->name;
                const size_t name_len = (size_t) dir_entry->name_len;

                // TODO: Ask: can we still assume 1 is the bad inode?
                if ((strncmp(file_name, dir_entry_name, name_len) == 0) && (dir_entry->inode >= 1)) {
                    return  dir_entry->inode;
                }
                cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;
            } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
        } else {
            break;
        }
    }
    return -1;
}


/* Search the directory inode dir_inode for the file with the name of file_name.
 * Returns the inode number of the file found, -1 if the file was not found or 
 * dir_inode is not a directory.
 */
unsigned int search_dir(char *file_name, struct ext2_inode *dir_inode){
    unsigned int inode_num_file_name;

    if((inode_num_file_name = search_dir_direct_blks(file_name, dir_inode)) == -1) {

        // Search remaining the indirect blocks
        // TODO: Ask: can we still assume 1 is the bad inode?
        
        if (dir_inode->i_block[12] >= 1) {
            struct ext2_inode *indirect_dir_inode = inode_table + dir_inode->i_block[12];
        }
    }
}

/* Given a path as a string, return the inode number of the 
 * last file in the path. -1 if path invalid.
 */
unsigned int get_inode_num(char *path, unsigned int relative_path_inode){
	
    // Assume path begins with a slash
    char *trimmed_root = path + 1;
    char *end_ptr;
	char dir_str[DIR_STR_LEN];
    unsigned int next_inode;
    int trimmed_len;

    if ((trimmed_len = strlen(trimmed_root)) == 0){
        return relative_path_inode;
    }

	if ((end_ptr = strchr(trimmed_root, '/')) != NULL){
	   strncpy(dir_str, trimmed_root, end_ptr - trimmed_root);
	} else {
        strncpy(dir_str, trimmed_root, DIR_STR_LEN);
        end_ptr = dir_str + trimmed_len - 1;
    }

    if ((next_inode = search_dir(dir_str, relative_path_inode)) == -1){
        return -1;
    }

    return get_inode_num(end_ptr, next_inode);

}
