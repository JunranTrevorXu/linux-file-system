#define FUSE_USE_VERSION 26

#ifdef linux
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <sys/statfs.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif


#define BLOCK_SIZE 512
struct superblock {
    uint32_t s_inodes_count; /* total number of inodes (used and free) */
    uint32_t s_blocks_count; /* total number of blocks (used and free) */ 
    uint32_t s_free_inodes_count; /* total number of free inodes */
    uint32_t s_free_blocks_count; /* total number of free data blocks */
    uint32_t s_first_data_block; /* which block is the first data block */
    uint32_t s_first_ino; /* index to first inode thats non-reserved */
    uint32_t s_magic;   /* Magic Signature is 0x554e4958 */
    /* remaining bytes are unused*/
};

struct inode {
    uint16_t i_mode;          /* File type (S_ISREG or S_ISDIR) and Permissions */
    uint16_t i_uid;           /* File owner */
    uint16_t i_gid;           /* File group */
    uint16_t i_links_count;   /* How many hard links are there to this file */
    uint32_t i_size;          /* Number of bytes in the file */
    uint32_t i_time;          /* Last access time */
    uint32_t i_ctime;         /* Creation time */
    uint32_t i_mtime;         /* Last modified time */
    uint32_t i_dtime;         /* When was this inode deleted */
    uint32_t i_blocks;        /* How many blocks are allocated to this file */

    uint32_t i_block[8];
    /* pointers to the blocks of data. (which datablock from first)
     * All point to direct blocks
     * */
};

/*
 * A directory should have 2 default entries when starting
 * first: a '.' dir pointing to itself
 * second: a '..' dir pointing to the parent directory
 * Also, a directory shouldn't fill in more than one block
 */
struct directory_entry {
    uint32_t        d_inode;        /* inode number */ 
    uint16_t        d_file_type;    /* 1 for regular file, 2 for directory */
    uint8_t         d_name_len;     /* length of file name */
    char            d_name[57]   ;    /* file name 0-57 bytes*/
};


int errno; 
FILE *fp;
struct superblock sb;
unsigned char block_bm[BLOCK_SIZE];
unsigned char inode_bm[BLOCK_SIZE];
extern unsigned int INO_SIZE;
extern unsigned int DIR_ENTRY_SIZE;

