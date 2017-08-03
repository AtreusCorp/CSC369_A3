#include "disk_routines.h"

/* Returns a pointer to the inode in the memory mapped into by
 * disk corresponding to inode_num.
 */
struct ext2_inode *fetch_inode_from_num(unsigned int inode_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    struct ext2_inode *inode_table = (struct ext2_inode *)(disk + group_desc->bg_inode_table 
                                                                  * EXT2_BLOCK_SIZE);
    return inode_table + inode_num - 1;
}

/* Returns 1 if the inode with inode_num is in use, 0 otherwise.
 */
int check_inode_bitmap(int inode_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *inode_bitmap = disk + group_desc->bg_inode_bitmap * EXT2_BLOCK_SIZE;

    int inode_index = inode_num - 1;
    int byte_index = inode_index / 8;
    int bit_in_byte = inode_index % 8;
    return (1 << bit_in_byte) & (*(inode_bitmap + byte_index));
}

/* Sets the inode bitmap to 1 for the inode corresponding to inode_num
 */
void set_inode_bitmap(int inode_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *inode_bitmap = disk + group_desc->bg_inode_bitmap * EXT2_BLOCK_SIZE;

    int inode_index = inode_num - 1;
    int byte_index = inode_index / 8;
    int bit_in_byte = inode_index % 8;
    *(inode_bitmap + byte_index) |= (1 << bit_in_byte);
    return;
}

/* Sets the inode bitmap to 0 for the inode corresponding to inode_num
 */
void unset_inode_bitmap(int inode_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *inode_bitmap = disk + group_desc->bg_inode_bitmap * EXT2_BLOCK_SIZE;

    int inode_index = inode_num - 1;
    int byte_index = inode_index / 8;
    int bit_in_byte = inode_index % 8;
    *(inode_bitmap + byte_index) &= ~(1 << bit_in_byte);
    return;
}

/* Returns 1 if the block with block_num is in use, 0 otherwise.
 */
int check_block_bitmap(int block_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *block_bitmap = disk + group_desc->bg_block_bitmap * EXT2_BLOCK_SIZE;

    int byte_index = block_num / 8;
    int bit_in_byte = block_num % 8;
    return (1 << bit_in_byte) & (*(block_bitmap + byte_index));
}

/* Sets the block bitmap to 1 for the block corresponding to block_num
 */
void set_block_bitmap(int block_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *block_bitmap = disk + group_desc->bg_block_bitmap * EXT2_BLOCK_SIZE;

    int byte_index = block_num / 8;
    int bit_in_byte = block_num % 8;
    *(block_bitmap + byte_index) |= (1 << bit_in_byte);
    return;
}

/* Sets the block bitmap to 0 for the block corresponding to block_num
 */
void unset_block_bitmap(int block_num){
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    unsigned char *block_bitmap = disk + group_desc->bg_block_bitmap * EXT2_BLOCK_SIZE;

    int byte_index = block_num / 8;
    int bit_in_byte = block_num % 8;
    *(block_bitmap + byte_index) &= ~(1 << bit_in_byte);
    return;
}

/* Search the directory inode dir_inode for the file with the name of file_name.
 * Returns the inode number of the file found, -1 if the file was not found or 
 * dir_inode is not a directory.
 */
int search_dir(char * file_name, struct ext2_inode *dir_inode) {
    int i;
    unsigned char *first_entry;
    unsigned char *cur_entry;
    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);

    if (S_ISDIR(dir_inode->i_mode) != 1){
        return -1;
    }

    for(i = 0; i < 12; ++i){
        if (!(check_block_bitmap(dir_inode->i_block[i]))
            || dir_inode->i_block[i] <= super_block->s_first_data_block){
            
            return -1;
        }

        first_entry = disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE;
        cur_entry = first_entry;

        do {
            struct ext2_dir_entry_2 *dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);
            const char *dir_entry_name = dir_entry->name;
            const size_t name_len = (size_t) dir_entry->name_len;

            // TODO: Ask: can we still assume 1 is the bad inode?
            if ((strncmp(file_name, dir_entry_name, name_len) == 0) 
                && strlen(file_name) == name_len
                && (dir_entry->inode >= 1)) {
                
                return  dir_entry->inode;
            }
            cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;
        } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
    }
    return -1;
}

/* Given a path as a string, return the inode number of the 
 * last file in the path. -1 if path invalid.
 */
int get_inode_num(char *path, unsigned int relative_path_inode_num){
	
    // Assume path begins with a slash
    char *trimmed_root = path + 1;
    char *end_ptr;
	char dir_str[EXT2_NAME_LEN];
    int next_inode;
    int trimmed_len;
    struct ext2_inode *relative_path_inode = 
                                fetch_inode_from_num(relative_path_inode_num);

    if ((trimmed_len = strlen(trimmed_root)) == 0){
        return relative_path_inode_num;
    }

	if ((end_ptr = strchr(trimmed_root, '/')) != NULL){
	    strncpy(dir_str, trimmed_root, end_ptr - trimmed_root + 1);
        dir_str[end_ptr - trimmed_root] = '\0';
	} else {
        strncpy(dir_str, trimmed_root, EXT2_NAME_LEN);
        dir_str[trimmed_len] = '\0';
        end_ptr = dir_str + trimmed_len;
    }

    if ((next_inode = search_dir(dir_str, relative_path_inode)) == -1){
        return -1;
    }

    return get_inode_num(end_ptr, next_inode);

}

/* Returns a free block number and sets it's corresponding bit map in 
 * the block bitmap to 1. -1 if no block could be allocated;
 */
int allocate_block(){
    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    int j = super_block->s_first_data_block;
    
    while(j < super_block->s_blocks_count){
        if(!check_block_bitmap(j)){

            memset((char *)(disk + j * EXT2_BLOCK_SIZE), 0, EXT2_BLOCK_SIZE);
            set_block_bitmap(j);
            super_block->s_free_blocks_count -= 1;
            group_desc->bg_free_blocks_count -= 1;
            return j;
        }
        ++j;
    }
    return -1;
}

/* Allocate a free inode for use along with an empty block. 
 * Returns the number of a cleared inode, -1 if none found.
 */
int allocate_inode(){
    
    int found_inode = -1;
    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)
                                            (disk + 2 * EXT2_BLOCK_SIZE);
    struct ext2_inode *inode_table = (struct ext2_inode *)(disk + group_desc->bg_inode_table 
                                                                  * EXT2_BLOCK_SIZE);
    unsigned int inode_count = super_block->s_inodes_count;
    int i = super_block->s_first_ino;
    int allocated_block;

    // Error here, use code from readimage
    while(i < inode_count && found_inode < 0){
        if(!check_inode_bitmap(i)){

            // inodes are indexed starting from 1
            memset(((char *)(inode_table + i - 1)), 0, sizeof(struct ext2_inode));
            found_inode = i;
        }
        ++i;
    }
    if (found_inode > -1){
        set_inode_bitmap(found_inode);

        // Allocate a free block and link it to the found inode
        if ((allocated_block = allocate_block()) < 0){
            printf("Error: Block Allocation failed.\n");
            return -1;
        }
        (inode_table + found_inode - 1)->i_block[0] = allocated_block;
        (inode_table + found_inode - 1)->i_blocks = 2;
        (inode_table + found_inode - 1)->i_size = EXT2_BLOCK_SIZE;
        (inode_table + found_inode - 1)->i_links_count = 1;
        group_desc->bg_free_inodes_count -= 1;
        super_block->s_free_inodes_count -= 1;
                
        return found_inode;
    }

    return -1;
}

unsigned char *allocate_dir_entry_slot(struct ext2_inode *p_inode, 
                                       struct ext2_dir_entry_2 *dir_entry){

    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    int i;
    int dir_size = sizeof(struct ext2_dir_entry_2) 
                   + sizeof(char) * dir_entry->name_len;
    int cur_block;
    int cur_dir_entry_size;
    int unallocated_gap_size;
    int unallocated_gap_size_alligned;
    unsigned char *first_entry;
    unsigned char *cur_entry;

    // Iterate through the direct 12 data blocks
    for(i = 0; i < 12; ++i){

        if (!check_block_bitmap(p_inode->i_block[i])
            || p_inode->i_block[i] <= super_block->s_first_data_block){

            if ((cur_block = allocate_block()) < 0){
                printf("Error: Block Allocation failed.\n");
                return NULL;
            }
            p_inode->i_block[i] = cur_block;
            p_inode->i_blocks += 2;
            p_inode->i_size += EXT2_BLOCK_SIZE;

        }
        first_entry = disk + p_inode->i_block[i] * EXT2_BLOCK_SIZE;
        cur_entry = first_entry;

        // Iterate through the ith block
        do {
            unsigned int new_rec_len;
            struct ext2_dir_entry_2 *cur_dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);

            if ((dir_entry->name_len == cur_dir_entry->name_len) 
                && (strncmp(dir_entry->name, cur_dir_entry->name, dir_entry->name_len) == 0)){
                printf("Error: File with the same name exists.\n");
                return NULL;
            }
            cur_dir_entry_size = sizeof(cur_dir_entry) + sizeof(char) * cur_dir_entry->name_len;
            unallocated_gap_size = cur_dir_entry->rec_len - cur_dir_entry_size;

            // 4 byte alignment
            unallocated_gap_size_alligned = unallocated_gap_size - (unallocated_gap_size % 4);

            // If the new entry can fit
            if(unallocated_gap_size_alligned >= dir_size){
                unsigned char *allignment_ptr = (unsigned char *) cur_dir_entry;
                new_rec_len = cur_dir_entry->rec_len;

                while(((cur_entry + cur_dir_entry_size) - allignment_ptr) > 0){
                    allignment_ptr += 4;
                    new_rec_len -= 4;
                }
                dir_entry->rec_len = new_rec_len;
                cur_dir_entry->rec_len = allignment_ptr - cur_entry;

                // Write the new entry into the data block
                ((struct ext2_dir_entry_2 *) allignment_ptr)->inode = dir_entry->inode;
                ((struct ext2_dir_entry_2 *) allignment_ptr)->rec_len = dir_entry->rec_len;
                ((struct ext2_dir_entry_2 *) allignment_ptr)->name_len = dir_entry->name_len;
                ((struct ext2_dir_entry_2 *) allignment_ptr)->file_type = dir_entry->file_type;
                strncpy(((struct ext2_dir_entry_2 *) allignment_ptr)->name, dir_entry->name, dir_entry->name_len);
                return allignment_ptr;

            // The case where the block is entirely empty
            } else if (cur_dir_entry->rec_len == 0){
                dir_entry->rec_len = EXT2_BLOCK_SIZE;
                cur_dir_entry->inode = dir_entry->inode;
                cur_dir_entry->rec_len = dir_entry->rec_len;
                cur_dir_entry->name_len = dir_entry->name_len;
                cur_dir_entry->file_type = dir_entry->file_type;
                strncpy(cur_dir_entry->name, dir_entry->name, dir_entry->name_len);
                return cur_entry;
            }
            cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;

        // While the whole block has not been covered
        } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
    }
    return NULL;
}

/* Search the directory with p_inode for the file with name victim_name 
 * and inode victim_inode_num, return a pointer to the corresponding 
 * directory entry, NULL if it was not found.
 */
struct ext2_dir_entry_2 *find_dir_entry(struct ext2_inode *p_inode, 
                              char *victim_name,
                              unsigned int victim_inode_num){

    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    int i;
    unsigned char *first_entry;
    unsigned char *cur_entry;

    // Iterate through the direct 12 data blocks
    for(i = 0; i < 12; ++i){
        if (!check_block_bitmap(p_inode->i_block[i])
            || p_inode->i_block[i] <= super_block->s_first_data_block){

            return NULL;
        }
        first_entry = disk + p_inode->i_block[i] * EXT2_BLOCK_SIZE;
        cur_entry = first_entry;

        // Iterate through the ith block
        do {
            struct ext2_dir_entry_2 *cur_dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);

            if ((cur_dir_entry->inode == victim_inode_num) 
                && (cur_dir_entry->name_len == strlen(victim_name)) 
                && (strncmp(victim_name, cur_dir_entry->name, cur_dir_entry->name_len) == 0)){
                return cur_dir_entry;
            }
            cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;

        // While the whole block has not been covered
        } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
    }
    return NULL;
}

/* Search the directory with p_inode for the file with name victim_name 
 * and inode victim_inode_num, return a pointer to the directory entry 
 * immediately before the entry with the given specifications. NULL if 
 * it was not found or if the specified entry lies at the beginning of 
 * a block.
 */
struct ext2_dir_entry_2 *find_prev_dir_entry(struct ext2_inode *p_inode, 
                                   char *victim_name,
                                   unsigned int victim_inode_num){

    struct ext2_super_block *super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    int i;
    unsigned char *first_entry;
    unsigned char *cur_entry;
    struct ext2_dir_entry_2 *prev_dir_entry; 

    // Iterate through the direct 12 data blocks
    for(i = 0; i < 12; ++i){
        if (!check_block_bitmap(p_inode->i_block[i])
            || p_inode->i_block[i] <= super_block->s_first_data_block){
            return NULL;
        }
        first_entry = disk + p_inode->i_block[i] * EXT2_BLOCK_SIZE;
        cur_entry = first_entry;
        prev_dir_entry = NULL;

        // Iterate through the ith block
        do {
            struct ext2_dir_entry_2 *cur_dir_entry = ((struct ext2_dir_entry_2 *) cur_entry);

            if ((cur_dir_entry->inode == victim_inode_num) 
                && (cur_dir_entry->name_len == strlen(victim_name)) 
                && (strncmp(victim_name, cur_dir_entry->name, cur_dir_entry->name_len) == 0)){
                return prev_dir_entry;
            }
            prev_dir_entry = cur_dir_entry;
            cur_entry += ((struct ext2_dir_entry_2 *) cur_entry)->rec_len;

        // While the whole block has not been covered
        } while ((cur_entry - first_entry) % EXT2_BLOCK_SIZE != 0);
    }
    return NULL;
}

int insert_cur_and_parent_dir(unsigned int p_inode_num, 
                                       struct ext2_inode *cur_inode,
                                       unsigned int cur_inode_num){

    struct ext2_dir_entry_2 *new_entry = calloc(1, sizeof(struct ext2_dir_entry_2) 
                                                + sizeof(char) * 2);
    new_entry->file_type |= EXT2_S_IFDIR;
    new_entry->name_len = 1;
    new_entry->inode = cur_inode_num;
    strncpy(new_entry->name, ".", 1);

    if(allocate_dir_entry_slot(cur_inode, new_entry) == NULL){
        printf("Error: dir entry allocation failed.\n");
            free(new_entry);
            return -1;
    }
    ++cur_inode->i_links_count;
    new_entry->name_len = 2;
    new_entry->inode = p_inode_num;
    strncpy(new_entry->name, "..", 2);

    if(allocate_dir_entry_slot(cur_inode, new_entry) == NULL){
        printf("Error: dir entry allocation failed.\n");
            free(new_entry);
            return -1;
    }
    ++fetch_inode_from_num(p_inode_num)->i_links_count;
    free(new_entry);
    return 0;
}

/* Insert an ext2_dir_entry_2 into p_inode and equip it with provided e_inode_num,
 * name_len, file_type, and name. If e_inode_num = 0, allocates a free inode and
 * equips the entry with that. Automatically computes rec_len. Returns 0 on success, 
 * -1 otherwise.
 */
int insert_dir_entry(struct ext2_inode *p_inode, 
                              unsigned int parent_dir_inode_num,
                              int e_inode_num,
                              unsigned char name_len,
                              unsigned char file_type,
                              char *name){
    struct ext2_inode *allocated_inode;
    struct ext2_dir_entry_2 *new_entry = malloc(sizeof(struct ext2_dir_entry_2) + sizeof(char) * name_len);
    new_entry->file_type = file_type;
    new_entry->name_len = name_len;
    strncpy(new_entry->name, name, name_len);

    // Allocated a new inode
    if (e_inode_num == 0){
        if((e_inode_num = allocate_inode()) < 0){
            printf("Error: inode allocation failed.\n");
            return -1;
        }
    }
    new_entry->inode = e_inode_num;

    allocated_inode = fetch_inode_from_num(e_inode_num);
    if (file_type == EXT2_FT_REG_FILE){
        allocated_inode->i_mode |= EXT2_S_IFREG;

    } else if (file_type == EXT2_FT_DIR){
        allocated_inode->i_mode |= EXT2_S_IFDIR;

    } else if (file_type == EXT2_FT_SYMLINK){
        allocated_inode->i_mode |= EXT2_S_IFLNK;

    } else {
        printf("Error: Could not handle file type.\n");
        free(new_entry);
        return -1;
    }

    if(allocate_dir_entry_slot(p_inode, new_entry) == NULL){

        printf("Error: dir entry allocation failed.\n");
            free(new_entry);
            return -1;
    }

    // Set up the directory properly
    if (file_type == EXT2_FT_DIR){
        insert_cur_and_parent_dir(parent_dir_inode_num, allocated_inode, e_inode_num);
    }
    free(new_entry);
    return 0;
}

int remove_dir_entry(struct ext2_inode *p_inode, 
                     char *victim_name,
                     unsigned int victim_inode_num){
    struct ext2_dir_entry_2 *victim_dir_entry = find_dir_entry(p_inode, 
        victim_name, 
        victim_inode_num);
    struct ext2_dir_entry_2 *victim_dir_entry_prev = find_prev_dir_entry(p_inode, 
        victim_name, 
        victim_inode_num);

    // If victim_dir_entry is not the first entry
    if ((victim_dir_entry != NULL) && (victim_dir_entry_prev != NULL)){
        victim_dir_entry->inode = 0;
        victim_dir_entry->name_len = 0;
        victim_dir_entry->file_type = EXT2_FT_UNKNOWN;
        victim_dir_entry_prev->rec_len += victim_dir_entry->rec_len;
        victim_dir_entry->rec_len = 0;

    // If victim_dir_entry is the first entry
    } else if ((victim_dir_entry != NULL) && (victim_dir_entry_prev == NULL)){
        victim_dir_entry->inode = 0;
        victim_dir_entry->name_len = 0;
        victim_dir_entry->file_type = EXT2_FT_UNKNOWN;
        victim_dir_entry->rec_len = EXT2_BLOCK_SIZE;
    } else {
        printf("Error: Directory entry not found.\n");
        return -1;
    }
    return 0;
}