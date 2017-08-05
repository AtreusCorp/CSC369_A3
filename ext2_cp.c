#include "disk_routines.h"
#include <sys/stat.h>

// Pointers to files and data structures
unsigned char *disk;
FILE *src_file_stream;
struct stat src_file_stat;
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;

// source related names
char src_path[EXT2_NAME_LEN + 1];
char *src_dir_end;
char src_filename[EXT2_NAME_LEN + 1];

// destination related names
char dest_path[EXT2_NAME_LEN + 1];
int dest_path_inode_num;
struct ext2_inode *dest_path_inode;
char dest_path_cpy[EXT2_NAME_LEN + 1];
char dest_pdir_pathname[EXT2_NAME_LEN + 1];
char dest_filename[EXT2_NAME_LEN + 1];
char *dest_pdir_end;
size_t dest_path_len;

// help functions
int map_disk(char *);
void set_filestream(char *);
size_t copy_stream_to_inode(FILE *filestream, struct ext2_inode *dest_inode);

// names for indices into argv
size_t img_index = 1;
size_t src_path_index = 2;
size_t dest_path_index = 3;

// help functions defintions
int map_disk(char *img_path) {
    if((disk = mmap(NULL,
                    128 * 1024,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    open(img_path, O_RDWR),
                    0)
       ) == MAP_FAILED){
        return -1;
    }
    return 0;
}

void set_filestream(char *file_path){
    int fd;

    fstat((fd=open(file_path, O_RDONLY)), &src_file_stat);
    if (src_file_stat.st_size > 2*12*EXT2_BLOCK_SIZE){
        printf("Error: %s's size is larger than 13 blocks", file_path);
        exit(-1);
    }
    close(fd);

    if ((src_file_stream = fopen(file_path, "r")) == NULL) {
        printf("%s: No such file or directory\n", file_path);
        exit(-1);
    }
}

// Copy data from src filestream, by EXT2_BLOCK_SIZE increament, until we get to eof of filestream
size_t copy_stream_to_inode(FILE* filestream, struct ext2_inode *dest_inode) {
    unsigned char block_buffer[EXT2_BLOCK_SIZE];
    int allocated_block_num;
    size_t total_bytes_read = 0;

    for (size_t blk_counter = 0; blk_counter < 12; ++blk_counter) {
        if ((total_bytes_read += fread(block_buffer, 1, EXT2_BLOCK_SIZE, src_file_stream)) % EXT2_BLOCK_SIZE == 0) {
            // Allocate a new data block to dest_inode, if needed
            if ((allocated_block_num = allocate_block()) < 0){
                printf("Error: Block Allocation failed.\n");
                exit(-1);
            }
            assign_block_inode(dest_inode, (unsigned) blk_counter, (unsigned) allocated_block_num);
            memcpy((disk + allocated_block_num * EXT2_BLOCK_SIZE), block_buffer, EXT2_BLOCK_SIZE);
            memset(block_buffer, 0, EXT2_BLOCK_SIZE);
        } else {
            // We have finished copying data already
            return total_bytes_read;
        }
    }

    // for as long as source file ptr has not reached the eof:
    // copy blk by blk from src file ptr to data blocks for the first 12 blocks
    // For 13th block, get an inode and allocate the 1st block for that inode

    // END Copy data from src filestream to data blocks pointed by dest_path_inode
}

int main(int argc, char **argv){

    // map the disk into memory
    if (map_disk(argv[img_index]) < 0) {
       exit(-1);
    }

    // map the maximum file size. This utility can handle and check if original file on host exist
    set_filestream(argv[src_path_index]);

    // --- BEGIN Error checking ---

    // assignments for names related to dest and src
    strncpy(dest_path, argv[dest_path_index], EXT2_NAME_LEN);
    strncpy(src_path, argv[src_path_index], EXT2_NAME_LEN);
    dest_path_len = strnlen(dest_path, EXT2_NAME_LEN) + 1;
    dest_path_inode_num = get_inode_num(dest_path, EXT2_ROOT_INO);
    dest_path_inode = fetch_inode_from_num((unsigned) dest_path_inode_num);

    if (dest_path_inode_num < 0 && dest_path[dest_path_len - 2] == '/') {
        // dest_path is a directory. It follows that we should insert to dest_path, but it dne.
        printf("%s: The destination directory doesn't exist.\n", dest_path);
        return ENOENT;

    } else if(dest_path_inode_num >= 1 && dest_path[dest_path_len - 2] != '/') {
        // dest_path is a file. It follows that we should insert dest_filename to dest_pdir_pathname.
        // But file already exists
        printf("%s: The destination file already exist.\n", dest_path);
        return ENOENT;
    }

    // Check if disk is full
    super_block = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    group_desc = (struct ext2_group_desc *) (disk + 2 * EXT2_BLOCK_SIZE);
    if (super_block->s_free_blocks_count*EXT2_BLOCK_SIZE < src_file_stat.st_size) {
        printf("Disk will be full before copying.\n");
        exit(-1);
    }

    // --- END Error checking ---


    // --- BEGIN Names assignment ---

    // To insert a name into destination directory,
    // we need destination file name, referenced by dest_filename
    // we need destination directory's full path, referenced by dest_pdir_pathname
    if (dest_path[dest_path_len - 2] != '/') {
        strncpy(dest_path_cpy, dest_path, dest_path_len);
        // assignments for names related to dest
        if ((dest_pdir_end = strrchr(dest_path_cpy, '/')) == NULL) {
            printf("Error: Processing destination file name");
            exit(-1);
        }
        strncpy(dest_filename, dest_pdir_end + 1, EXT2_NAME_LEN);
        *(dest_pdir_end + 1) = '\0';
        strncpy(dest_pdir_pathname, dest_path_cpy, EXT2_NAME_LEN);

    } else {
        strncpy(dest_pdir_pathname, dest_path, dest_path_len);
        // assignment for dest_filename
        if ((src_dir_end = strrchr(src_path, '/')) == NULL){
            printf("Error: Processing source file name");
            exit(-1);
        }
        strncpy(src_filename, src_dir_end + 1, EXT2_NAME_LEN);
        strncpy(dest_filename, src_filename, EXT2_NAME_LEN);
    }

    // --- END names assignment ---


    // insert dest_filename as dir entry into dest_pdir_pathname
    if(insert_dir_entry(dest_path_inode,
                        dest_path_inode_num,
                        0,
                        strlen(dest_filename) + 1,
                        EXT2_FT_REG_FILE,
                        dest_filename) < 0) {
        perror("Error: Problem encountered when inserting directory entry.");
        exit(-1);
    }

    // BEGIN copying data

    size_t bytes_copied = copy_stream_to_inode(src_file_stream, dest_path_inode);
    if (bytes_copied < src_file_stat.st_size) {
        // Need 1 level of indirection here
        dest_path_inode->i_block[12] = (unsigned) allocate_inode();
        bytes_copied += copy_stream_to_inode(src_file_stream, fetch_inode_from_num(dest_path_inode->i_block[12]));
        if (bytes_copied != src_file_stat.st_size) {
            printf("Bytes read does not match with file size");
            exit(-1);
        }
    }

    // END copying data


    return 0;
}
