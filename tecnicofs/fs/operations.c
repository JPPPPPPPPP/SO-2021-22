#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO locks for inodes & data blocks
        make locks for open files
*/

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Truncate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) 
            {
                if(inode_delete_all_blocks(inode) == -1)
                {
                    return -1;
                }
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to write */
    if (to_write + file->of_offset > MAX_FILE_SIZE) {
        to_write = MAX_FILE_SIZE - file->of_offset;
    }

    if (to_write > 0) {
        size_t final_size = file->of_offset + to_write;
        size_t written = 0;
        for(size_t block_idx = inode->i_size/BLOCK_SIZE; block_idx <= final_size/BLOCK_SIZE; block_idx++) 
        {
            size_t block_offset = file->of_offset % BLOCK_SIZE;
	        
            //makes sure the block is only partially written into
            size_t to_write_in_block = BLOCK_SIZE - block_offset;
            if(to_write_in_block > to_write - written) 
            {
                to_write_in_block = to_write - written;
            }
            //checks if to_write_in_block is empty and breaks the loop so it doesnt allocate an empty block
            if(to_write_in_block == 0) 
            {
                break;
            }
            
            int temp = get_block_from_idx(inode, block_idx, 1);
            if(temp == -1)
            {
                return -1;
            }
            void* block = data_block_get(temp);
            if(block == NULL) 
            {
                return -1;
            }
            
            /* Perform the actual write */
            memcpy(block + block_offset, buffer + written, to_write_in_block);
	        written += to_write_in_block;
        }

        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset = final_size;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)to_write;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (file->of_offset + to_read >= MAX_FILE_SIZE) {
        return -1;
    }

    if (to_read > 0) {
        size_t final_size = file->of_offset + to_read;
        size_t read = 0;
        for(size_t block_idx = file->of_offset / BLOCK_SIZE; block_idx <= final_size/BLOCK_SIZE; block_idx++) 
        {
            //finds the offset in the block rather than file
			size_t block_offset = file->of_offset % BLOCK_SIZE;
	        
            //makes sure the block is only partially read from
            size_t to_read_in_block = BLOCK_SIZE - block_offset;
            if(to_read_in_block > to_read - read) 
            {
                to_read_in_block = to_read - read;
            }
            
            //checks if to_read_in_block is empty and breaks the loop
            if(to_read_in_block == 0) 
            {
                break;
            }

            int temp = get_block_from_idx(inode, block_idx, 0);
            if(temp == -1)
            {
                return -1;
            }
            void* block = data_block_get(temp);
            if(block == NULL) 
            {
                return -1;
            }

            /* Perform the actual read */
	        memcpy(buffer + read, block + block_offset, to_read_in_block);
	        read += to_read_in_block;
        }

        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset = final_size;
    }

    return (ssize_t)to_read;
}


int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) 
{
    int tfs_d = tfs_open(source_path, 0);
    if(tfs_d == -1) 
    {
        return -1;
    }

    open_file_entry_t *file = get_open_file_entry(tfs_d);
    if (file == NULL) {
        return -1;
    }

    //gets inode
    inode_t *inode = inode_get(file->of_inumber);
    if(inode == NULL) 
    {
        return -1;
    }

    size_t len = inode->i_size;
    char* contents = (char*)malloc(sizeof(char)*(len+1));
    tfs_read(tfs_d, contents, len);
    contents[len] = '\0';

    FILE *fd = fopen(dest_path, "wb");
    if(fd == NULL) 
    {
        free(contents);
        return -1;
    }
    if(fwrite(contents, 1, len, fd) != len) 
    {
        fclose(fd);
        free(contents);
        return -1;
    }

    fclose(fd);
    free(contents);
    return 0;
}