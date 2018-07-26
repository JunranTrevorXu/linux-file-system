#include "fusefs.h"

unsigned int INO_SIZE = sizeof(struct inode);
unsigned int DIR_ENTRY_SIZE = sizeof(struct directory_entry);

#define BLK_BM_POS 1
#define INO_BM_POS 2
#define INO_TA_POS 3
// Helper: checks if a file pointer is still valid
void checkFilePointer(FILE *fp)
{
    if (fp == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
}

// Given updates the first block with the most recent superblock
void write_superblock()
{
    checkFilePointer(fp);
    unsigned char buffer[BLOCK_SIZE] = {0};
    int i;
    // Write the super block to the first block in the image
    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(struct superblock), 1, fp);
    for (i = 0; i < BLOCK_SIZE - sizeof(struct superblock); i++) {
        fputc(buffer[i], fp);
    }
}

// Checks if a bit is set in a bitmap
bool is_bit_set(unsigned char *bitmap, unsigned int bit)
{
    if (bit >= BLOCK_SIZE * 8) {
        perror("Invalid bit location while checking some bitmap");
        exit(-1);
    }

    // Calculate bit position
    unsigned int position = bit / 8;
    unsigned int offset = bit % 8;
    bool is_set = (bitmap[position] & (1 << offset));

    return is_set;
}

// Used to update block and inode bitmaps in memory
void update_bitmap(unsigned char *bitmap, unsigned int bit, bool value)
{
    if (bit >= BLOCK_SIZE * 8) {
        perror("Invalid bit location while updating bitmap");
        exit(-1);
    }
    unsigned int position = bit / 8;
    unsigned int offset = bit % 8;

    if (value == true)
        bitmap[position] |= (1 << (offset));
    else
        bitmap[position] &= ~(1 << (offset));
}

void update_block_bitmap(unsigned int bit, bool value)
{
    checkFilePointer(fp);
    // Update local memory copy
    update_bitmap(block_bm, bit, value);
    // Update disk copy
    fseek(fp, (bit / 8) + (BLOCK_SIZE * BLK_BM_POS), SEEK_SET);
    fputc(block_bm[bit / 8], fp);
}

void update_inode_bitmap(unsigned int bit, bool value)
{
    checkFilePointer(fp);
    // Update local memory copy
    update_bitmap(inode_bm, bit, value);
    // Update disk copy
    fseek(fp, (bit / 8) + (BLOCK_SIZE * INO_BM_POS), SEEK_SET);
    fputc(inode_bm[bit / 8], fp);
}

// Returns inode id of a newly created inode
uint32_t create_inode(struct inode *new_inode)
{
    checkFilePointer(fp);
    if(sb.s_free_inodes_count == 0) { // Make sure we have an inode free
        perror("No more free inodes to use!\n");
        exit(EXIT_FAILURE);
    }

    // index of next available inode
    unsigned int ino_number = sb.s_first_ino;

    // Get the first available inode
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        if (is_bit_set(inode_bm, i) != true) {
            ino_number = i;
            break;
        }
    }

    if (is_bit_set(inode_bm, ino_number) == true) { // Sanity check
        perror("Creating an inode that already exists");
        exit(EXIT_FAILURE);
    }

    // Save location in the inode table
    unsigned int ino_tab_loc = (ino_number * INO_SIZE) + (BLOCK_SIZE * INO_TA_POS);

    // Reserve the inode in the bitmap and write it to the table
    update_inode_bitmap(ino_number, true);
    fseek(fp, ino_tab_loc, SEEK_SET);
    new_inode->i_time = time(NULL);
    new_inode->i_mtime = time(NULL);
    new_inode->i_ctime = time(NULL);
    fwrite(new_inode, INO_SIZE, 1, fp);
    // update superblock to reflect changes
    sb.s_free_inodes_count--;
    write_superblock();

    return ino_number;
}

// Replace inode with new information
void update_inode(unsigned int inode_number, struct inode new_inode)
{
    checkFilePointer(fp);

    if (is_bit_set(inode_bm, inode_number) == false) {
        perror("Tried to update an inode that doesn't exists");
        exit(EXIT_FAILURE);
    }

    // Location of inode
    unsigned int ino_tab_loc =
        (inode_number * INO_SIZE) + (BLOCK_SIZE * INO_TA_POS);

    new_inode.i_mtime = time(NULL);

    // Save changes to disk
    fseek(fp, ino_tab_loc, SEEK_SET);
    fwrite(&new_inode, INO_SIZE, 1, fp);
}

// Returns inode from inode table
struct inode read_inode(unsigned int inode_number)
{
    /* out of bounds check */
    if (inode_number > sb.s_inodes_count || inode_number < sb.s_first_ino) {
        perror("Getting an invalid inode: out of range");
        exit(EXIT_FAILURE);
    } else if (is_bit_set(inode_bm, inode_number) == false) {
        perror("Getting an invalid inode: not in bitmap");
        exit(EXIT_FAILURE);
    }

    checkFilePointer(fp);
    // Location of inode
    unsigned int ino_tab_loc =
        (inode_number * INO_SIZE) + (BLOCK_SIZE * INO_TA_POS);

    struct inode temp;
    // Read inode from disk
    fseek(fp, ino_tab_loc, SEEK_SET);
    fread(&temp, INO_SIZE, 1, fp);
    return temp;
}

// returns a new data blocks location
unsigned int get_free_block()
{
    checkFilePointer(fp);
    if(sb.s_free_blocks_count == 0) { // Make sure we have block free
        perror("No more free blocks to use");
        exit(EXIT_FAILURE);
    }

    // index of block
    unsigned int block_number = 0;

    // Get the first available block
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        if (is_bit_set(block_bm, i) != true) {
            block_number = i;
            break;
        }
    }

    if (is_bit_set(block_bm, block_number) == true) { // Sanity check
        perror("Creating a block that already exists");
        exit(EXIT_FAILURE);
    }

    // Reserve the block in the bitmap and write it to the table
    update_block_bitmap(block_number, true);
    // update superblock to reflect changes
    sb.s_free_blocks_count--;
    write_superblock();

    return block_number;
}

// write to a valid block of memory.
void write_block(unsigned int block_number, unsigned char *buffer)
{
    checkFilePointer(fp);
    if(!is_bit_set(block_bm, block_number)) {
        perror("Writing to an invalid block");
        exit(EXIT_FAILURE);
    }

    unsigned int block_location =
        (block_number + sb.s_first_data_block) * BLOCK_SIZE;

    fseek(fp, block_location, SEEK_SET);
    fwrite(buffer, BLOCK_SIZE, 1, fp);
}

// Puts the contents of block at block_number into buffer
void read_block(unsigned int block_number, unsigned char *buffer)
{
    checkFilePointer(fp);
    // Check if the block even exists
    if (is_bit_set(block_bm, block_number) == false) {
        perror("Getting block contents of non-existing block");
        exit(EXIT_FAILURE);
    }

    unsigned int block_location =
        (block_number + sb.s_first_data_block) * BLOCK_SIZE;
    fseek(fp, block_location, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, fp);
}

// puts the list of directory entires into parameter, returns num of entries
unsigned int read_dir_node(
        struct inode dir_inode,
        struct directory_entry *dir_list)
{
    /* Assumed dir_list can hold an entire blocks worth */
    checkFilePointer(fp);
    // Check if if we are reading a directory to begin with
    if (!S_ISDIR(dir_inode.i_mode)) {
        perror("Attempting reading dir of a non-directory inode");
        exit(EXIT_FAILURE);
    }

    struct directory_entry *entry;
    unsigned char block_contents[BLOCK_SIZE];
    unsigned int num_contents = dir_inode.i_size / DIR_ENTRY_SIZE;

    // grab data block
    read_block(dir_inode.i_block[0], block_contents);
    entry = (struct directory_entry *) block_contents; // First file in dir

    for (int i = 0; i < num_contents; i++) {
        dir_list[i] = *entry;
        entry++; // Get next entry
    }
    return num_contents;
}

// Puts the contents of the file into buffer
void read_file_node(unsigned int inode_number, unsigned char *buffer)
{
    checkFilePointer(fp);

    struct inode file_inode = read_inode(inode_number);;
    /* assumed that the buffer is large enough to hold the files contents */
    // Check if if we are reading a directory to begin with
    if (!S_ISREG(file_inode.i_mode)) {
        perror("Attempting reading (as file) a non-regular file");
        exit(EXIT_FAILURE);
    }
    unsigned char block[BLOCK_SIZE];

    int i, j;
    for (i = 0; i < file_inode.i_blocks; i++) {
        read_block(file_inode.i_block[i], block);
        for (j = 0; (j < BLOCK_SIZE) && ((i * BLOCK_SIZE + j) < file_inode.i_size); j++) {
            buffer[i * BLOCK_SIZE + j] = block[j];
        }
    }
    // We accessed this inode now
    file_inode.i_time = time(NULL);
    update_inode(inode_number, file_inode);
}

// Deletes a file/folder at a specified inode
/*
void delete_inode(unsigned int inode_number, unsigned int parent_inode_index)
{   
    int i, j;
    checkFilePointer(fp);
    // Clear out the blocks of data from the bitmap of this inode
    struct inode temp = read_inode(inode_number);
    struct inode parent = read_inode(parent_inode_index);
    struct directory_entry p_list[(parent.i_size / DIR_ENTRY_SIZE)];

    unsigned int num_contents;
    unsigned char block[BLOCK_SIZE]; // temp buffer to write to disk

    // Clear data blocks
    for (i = 0; i < BLOCK_SIZE; i++) { block[i] = 0; }

    if (S_ISREG(temp.i_mode)) {
        temp.i_links_count--; // Delete this link
        if (temp.i_links_count < 1) { // dealloc this inode if no more links

            for (i = 0; (i < temp.i_blocks); i++) {
                write_block(temp.i_block[i], block);
                update_block_bitmap(temp.i_block[i], false);
            }
            // Metadata updates
            temp.i_dtime = time(NULL);
            update_inode(inode_number, temp); // updating on the inode table
            update_inode_bitmap(inode_number, false);

        } else { // We just update the inode with the removed link
            update_inode(inode_number, temp);
        }
    } else { // Folder. We need to recurse down.
        struct directory_entry dir_list[(temp.i_size / DIR_ENTRY_SIZE)];

        num_contents = read_dir_node(temp, dir_list);
        for (i = 2; i < num_contents; i++) {
            // Get rid of all the children (skipping "." and "..").
            delete_inode(dir_list[i].d_inode, inode_number);
        }
        for (i = 0; (i < temp.i_blocks); i++) {
            write_block(temp.i_block[i], block);
            update_block_bitmap(temp.i_block[i], false);
        }
        // Metadata updates
        temp.i_dtime = time(NULL);
        update_inode(inode_number, temp); // updating on the fs
        update_inode_bitmap(inode_number, false);
    }

    // delete this entry from p_list, and update parent.
    num_contents = read_dir_node(parent, p_list);
    j = 0;
    for (i = 0; i < num_contents; i++)
    {
        if(p_list[i].d_inode == inode_number)
        {// skip over the inode that we just deleted
            continue;
        }
        memcpy(&block[j * DIR_ENTRY_SIZE], &p_list[i], DIR_ENTRY_SIZE);
        j++;
    }

    write_block(parent.i_block[0], block);
    parent.i_size -= DIR_ENTRY_SIZE;
    parent.i_time = time(NULL);
    parent.i_mtime = time(NULL);
    update_inode(parent_inode_index, parent);

    // Update superblock
    sb.s_free_inodes_count++;
    sb.s_free_blocks_count += temp.i_blocks; // we freed those blocks
    write_superblock();
}
*/

// New directory creation. returns inode #
uint32_t make_dir_node(unsigned int parent_index, char *dir_name)
{
    bool isRoot = false; // sets to true if the dir we are making is '/'

    int i;
    struct inode dir_inode;
    unsigned char block[BLOCK_SIZE]; // temp buffer to write to disk

    // The new (child directory) inode
    uint32_t dirs_inode_entry = create_inode(&dir_inode);
    if(dirs_inode_entry == 2) // root dir points to itself
        isRoot = true;

    struct inode parent_inode;
    if (isRoot) {
        parent_inode = dir_inode;
    } else {
        parent_inode = read_inode(parent_index);

        // We can't make a directory under a regular file after all
        if(!S_ISDIR(parent_inode.i_mode)) {
            perror("mkdir: parent is not a directory\n");
            exit(EXIT_FAILURE);
        }
        if ((parent_inode.i_size) >= BLOCK_SIZE) {
            perror("mkdir: parent full\n");
            exit(EXIT_FAILURE);
        }
    }

    // Clear out the block that I'm going to write
    for (i = 0; i < BLOCK_SIZE; i++) {
        block[i] = 0;
    }

    dir_inode.i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    dir_inode.i_uid = getuid();
    dir_inode.i_gid = getgid();
    dir_inode.i_links_count = 1;

    dir_inode.i_size = 0;
    dir_inode.i_time =  time(NULL);
    dir_inode.i_ctime = time(NULL);
    dir_inode.i_mtime = time(NULL);
    dir_inode.i_blocks = 1; // The dir_entries list fits in a single block

    for (i = 0; i < 8; i++) // sanity check: clear out all the data blocks.
        dir_inode.i_block[i] = 0;

    // all our entries should fit in one block
    unsigned int dir_block = get_free_block();
    dir_inode.i_block[0] = dir_block;

    // Make the . and .. entries
    struct directory_entry parent_dir, this_dir;
    for (i = 0; i < 57; i++) { // clearing out the name (just in case).
        parent_dir.d_name[i] = 0;
        this_dir.d_name[i] = 0;
    }

    // . dir
    this_dir.d_inode = dirs_inode_entry;
    this_dir.d_file_type = 2;
    this_dir.d_name_len = strlen(".");
    strncpy(this_dir.d_name, ".", this_dir.d_name_len);
    // putting the '.' direntry in the beginning of the block
    memcpy(block, &this_dir, sizeof(this_dir));
    dir_inode.i_size += sizeof(this_dir); // Could have used DIR_ENTRY_SIZE.

    // .. dir
    if(isRoot) { // root dir points to itself
        parent_dir.d_inode = dirs_inode_entry;
    }
    else {
        parent_dir.d_inode = parent_index;
    }

    parent_dir.d_file_type = 2;
    parent_dir.d_name_len = strlen("..");
    strncpy(parent_dir.d_name, "..", parent_dir.d_name_len);
    memcpy(&block[dir_inode.i_size], &parent_dir, sizeof(parent_dir));
    dir_inode.i_size += sizeof(parent_dir);

    write_block(dir_block, block);
    update_inode(dirs_inode_entry, dir_inode);

    // Updating parents info here
    if (!isRoot) {
        struct directory_entry dir_list[(parent_inode.i_size / DIR_ENTRY_SIZE) + 1];
        // ^--- left room for the newly created child
        unsigned int num_contents = read_dir_node(parent_inode, dir_list);

        struct directory_entry temp;
        temp.d_inode = dirs_inode_entry;
        temp.d_file_type = 2;
        temp.d_name_len = strlen(dir_name);
        strncpy(temp.d_name, dir_name, 57);
        dir_list[num_contents] = temp;

        // Clear out the block that I'm going to write
        for (i = 0; i < BLOCK_SIZE; i++) {
            block[i] = 0;
        }

        for (i = 0; i < num_contents + 1; i++) {
            memcpy(&block[i * DIR_ENTRY_SIZE], &dir_list[i], DIR_ENTRY_SIZE);
        }

        write_block(parent_inode.i_block[0], block);
        parent_inode.i_size += DIR_ENTRY_SIZE;
        parent_inode.i_time = time(NULL);
        parent_inode.i_mtime = time(NULL);
        update_inode(parent_index, parent_inode);
    }
    return dirs_inode_entry;
}

// Make a new file with its contents at the parents location
uint32_t make_file_node(
        unsigned int parent_index,
        char *file_name,
        char *fileContents,
        unsigned int fileSize
        )
{
    struct inode parent_inode = read_inode(parent_index);

    int i, j;
    struct inode file_inode;

    // How many blocks do we need?
    unsigned int numblocks = (fileSize / BLOCK_SIZE) +
        ((fileSize % BLOCK_SIZE > 0) ? 1 : 0); // leaving room for remainder
    unsigned char blocks[numblocks][BLOCK_SIZE]; // temp buffer to write to disk
    // We can't make a file under a regular file after all
    if(!S_ISDIR(parent_inode.i_mode)) {
        perror("mkfile: parent is not a directory\n");
        exit(EXIT_FAILURE);
    }
    if ((parent_inode.i_size) >= BLOCK_SIZE) {
        perror("mkfile: parent full\n");
        exit(EXIT_FAILURE);
    }

    file_inode.i_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
    file_inode.i_uid = getuid();
    file_inode.i_gid = getgid();
    file_inode.i_links_count = 1;

    file_inode.i_size = fileSize;
    file_inode.i_time =  time(NULL);
    file_inode.i_ctime = time(NULL);
    file_inode.i_mtime = time(NULL);
    file_inode.i_blocks = numblocks;

    // Clear out the blocks that I'm going to write
    for (i = 0; i < numblocks; i++) {
        for (j = 0; j < BLOCK_SIZE; j++) {
            blocks[i][j] = 0;
        }
    }

    // And now copy the expected contents over
    for (i = 0; i < fileSize; i++ ) {
        blocks[i / BLOCK_SIZE][i % BLOCK_SIZE] = fileContents[i];
    }

    for (i = 0; i < 8; i++) // sanity check: clear out all the data pointers.
        file_inode.i_block[i] = 0;
    for (i = 0; i < numblocks; i++) { // and now we get the data blocks to use
        file_inode.i_block[i] = get_free_block();
        write_block(file_inode.i_block[i], blocks[i]);
    }

    // The new (child file) inode
    uint32_t file_inode_entry = create_inode(&file_inode);

    // Updating parents info here
    struct directory_entry dir_list[(parent_inode.i_size / DIR_ENTRY_SIZE) + 1];
    // ^--- left room for the newly created child
    unsigned int num_contents = read_dir_node(parent_inode, dir_list);

    struct directory_entry temp;
    temp.d_inode = file_inode_entry;
    temp.d_file_type = 1;
    temp.d_name_len = strlen(file_name);
    strncpy(temp.d_name, file_name, 57);
    dir_list[num_contents] = temp;

    unsigned char block[BLOCK_SIZE];
    // Clear out the block that I'm going to write
    for (i = 0; i < BLOCK_SIZE; i++) {
        block[i] = 0;
    }

    for (i = 0; i < num_contents + 1; i++) {
        memcpy(&block[i * DIR_ENTRY_SIZE], &dir_list[i], DIR_ENTRY_SIZE);
    }

    write_block(parent_inode.i_block[0], block);
    parent_inode.i_size += DIR_ENTRY_SIZE;
    parent_inode.i_time = time(NULL);
    parent_inode.i_mtime = time(NULL);
    update_inode(parent_index, parent_inode);
    return file_inode_entry;
}

void open_filesystem(char *real_path, unsigned int n)
{
    fp = fopen(real_path, "rb+");
    checkFilePointer(fp);

    fseek(fp, 0, SEEK_SET); // just making sure here

    // grabbing the data for our fs.
    fread(&sb, sizeof(struct superblock), 1, fp);
    fseek(fp, BLOCK_SIZE, SEEK_SET); // just making sure here
    fread(&block_bm, sizeof(char), BLOCK_SIZE, fp);
    fread(&inode_bm, sizeof(char), BLOCK_SIZE, fp);
}

