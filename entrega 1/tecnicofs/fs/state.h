#ifndef STATE_H
#define STATE_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

//macro to use in case of error. prints error message and exits
#define error(s) {fprintf(stderr, "%s\n", s); exit(1);}

#define init_lock(lock) {if(pthread_rwlock_init(lock, NULL) != 0) \
    { fprintf(stderr, "Lock could not be initialized at: %s\n", __func__); exit(1); }\
}

#define destroy_lock(lock) {if(pthread_rwlock_destroy(lock) != 0) \
    { fprintf(stderr, "Lock could not be destroyed at: %s\n", __func__); exit(1); }\
}

#define wrlock(lock) { if(pthread_rwlock_wrlock(lock) != 0)\
    { fprintf(stderr, "Lock could not be acquired at: %s\n", __func__); exit(1); }\
}

#define rdlock(lock) { if(pthread_rwlock_rdlock(lock) != 0)\
    { fprintf(stderr, "Lock could not be acquired at: %s\n", __func__); exit(1); }\
}

#define unlock(lock) { if(pthread_rwlock_unlock(lock) != 0)\
    { fprintf(stderr, "Lock could not be unlocked at: %s\n", __func__); exit(1); }\
}

/*
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY } inode_type;

/*
 * I-node
 */
typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block[DATA_BLOCK_COUNT];
    pthread_rwlock_t i_lock;  
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

/*
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
    pthread_rwlock_t of_lock;
} open_file_entry_t;

#define MAX_DIR_ENTRIES (BLOCK_SIZE / sizeof(dir_entry_t))

void state_init();
void state_destroy();

int inode_create(inode_type n_type);
void* locking_memcpy(void *restrict dest, const void *restrict source, size_t n);
int get_block_from_idx(inode_t *inode, size_t block_idx, int create_new);
int inode_delete_all_blocks(inode_t *inode);
int inode_delete(int inumber);
inode_t *inode_get(int inumber);

int clear_dir_entry(int inumber, int sub_inumber);
int add_dir_entry(int inumber, int sub_inumber, char const *sub_name);
int find_in_dir(int inumber, char const *sub_name);

int data_block_alloc();
int data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset);
int remove_from_open_file_table(int fhandle);
open_file_entry_t *get_open_file_entry(int fhandle);

#endif // STATE_H
