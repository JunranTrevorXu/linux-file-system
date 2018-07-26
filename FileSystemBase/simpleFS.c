#include "simpleFS.h"

int offset = 0;

// Initialize a filesystem of the size specified in number of data blocks
void init_filesystem(unsigned int size, char *real_path, unsigned int n)
{
    /*
     * You first need to initalize a superblock. Write that to disk.
     * Create the bitmaps for block_bm and inode_bm.
     * write both to disk
     * Make a 5 block large inode table.
     * Then initialize the "size" datablocks.
     *
     * Also, when this is all done. you should have a file system ready.
     * with a parent directory of '/' at inode 2.
     */
    int i;
    int j;

    fp = fopen(real_path, "w");

    if (fp == NULL) {

    	printf("ACE error: bad directory\n");
    	return;
    }

    sb.s_inodes_count = 40;
    sb.s_blocks_count = size + 8;
    sb.s_free_inodes_count = 37;
    sb.s_free_blocks_count = size - 1;
    sb.s_first_data_block = 8;
    sb.s_first_ino = 2;
    sb.s_magic = 0x554e4958;

    //superblock
    fwrite(&sb, 28, 1, fp);

    for (i = 0; i < 121; i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    //two bitmaps

    for (i = 0; i < BLOCK_SIZE; i++) {

        block_bm[i] = 0xff;
        inode_bm[i] = 0xff;
    }

    //printf("ace\n");

    for (i = 0; i < size / 8; i++) {

    	block_bm[i] = 0x00;
    	inode_bm[i] = 0x00;
    }

    //printf("ace\n");

    for (j = 0; j < size % 8; j++) {

    	block_bm[i] >>= 1;
    	inode_bm[i] >>= 1;
    }

    block_bm[0] = 0b10000000;
    inode_bm[0] = 0b11100000;

    //printf("ace\n");

    fwrite(block_bm, 512, 1, fp);
    fwrite(inode_bm, 512, 1, fp);

    //printf("ace\n");

    //inode table
    struct inode rtdir;
    
    rtdir.i_mode = umask((mode_t)(S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO));
    rtdir.i_uid = (uint16_t)getuid();
    rtdir.i_gid = (uint16_t)getgid();
    rtdir.i_links_count = 0;
    rtdir.i_size = 128;
    rtdir.i_time = (uint16_t)time(NULL);
    rtdir.i_ctime = (uint16_t)time(NULL);
    rtdir.i_mtime = (uint16_t)time(NULL);
    rtdir.i_dtime = 0;
    rtdir.i_blocks = 1;
    rtdir.i_block[0] = 0;


    for (i = 0; i < 32; i++) {

        fwrite(&offset, 4, 1, fp);
    }

    fwrite(&rtdir, 64, 1, fp);

    for (i = 0; i < 37 * 16; i++) {

        fwrite(&offset, 4, 1, fp);
    }

    //printf("ace\n");

    //data blocks
    //root data block
    struct directory_entry rtent;
    rtent.d_inode = 2;
    rtent.d_file_type = 2;
    rtent.d_name_len = 1;
    strncpy(rtent.d_name, ".\0", 57);
    fwrite(&rtent, 64, 1, fp);

    rtent.d_inode = 2;
    rtent.d_file_type = 2;
    rtent.d_name_len = 2;
    strncpy(rtent.d_name, "..\0", 57);
    fwrite(&rtent, 64, 1, fp);

    //printf("ace\n");

    //printf("%s\n", rtent.d_name);

    for (i = 0; i < 96; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
    //other data blocks

    //printf("ace\n");

    for (i = 0; i < 128 * (size - 1); i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    //printf("ace\n");

    fclose(fp);
}

void open_filesystem(char *real_path, unsigned int n)
{
    /*
     * You will need to open an existing file system image from disk.
     * Read the file system information from the disk and initialize the
     * in-memory variables to match what was on disk.
     * use this file system for everything from now on.
     * Fail if the magic signature doesn't match
     */
    int i;

    fp = fopen(real_path, "r+");

    if (fp == NULL) {

    	printf("ACE error: bad directory\n");
    	return;
    }

    char buffer[512];
    //read in sb
    fread(&sb, 28, 1, fp);
    fread(buffer, 484, 1, fp);

    if (sb.s_magic != 0x554e4958) {

        printf("ACE error: magic number does not match\n");
        return;
    }

    //read in block bm
    fread(block_bm, 512, 1, fp);
    fread(inode_bm, 512, 1, fp);

    //fclose(fp);
}

int int_pow(int a, int b) {

	int res = 1;

	for (int i = 0; i < b; i++) {

		res *= a;
	}

	return res;
}

void access_inode(uint32_t inode_number) {

	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	ino.i_time = (uint32_t)time(NULL);
	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	fwrite(&ino, 64, 1, fp);
}

void modify_inode(uint32_t inode_number) {

	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	ino.i_mtime = (uint32_t)time(NULL);
	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	fwrite(&ino, 64, 1, fp);
}

void delete_inode(uint32_t inode_number) {

	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	ino.i_mtime = (uint32_t)time(NULL);
	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	fwrite(&ino, 64, 1, fp);

	int i = inode_number / 8;
	int j = inode_number % 8;
	inode_bm[i] &= 0xff - int_pow(2, 7 - j);
	sb.s_free_inodes_count++;
	fseek(fp, 0, SEEK_SET);
	fwrite(&sb, 28, 1, fp);
	for (i = 0; i < 121; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
	fwrite(block_bm, 512, 1, fp);
	fwrite(inode_bm, 512, 1, fp);
	fseek(fp, 3*512+64*inode_number, SEEK_SET);
	for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
}

void delete_block(uint32_t block_number) {

	int i = block_number / 8;
	int j = block_number % 8;
	block_bm[i] &= 0xff - int_pow(2, 7 - j);
	sb.s_free_blocks_count++;
	fseek(fp, 0, SEEK_SET);
	fwrite(&sb, 28, 1, fp);
	for (i = 0; i < 121; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
	fwrite(block_bm, 512, 1, fp);
	fwrite(inode_bm, 512, 1, fp);
	fseek(fp, 8*512+512*block_number, SEEK_SET);
	for (i = 0; i < 128; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
}

int get_inode() {

	int inode_number;
	int i;
	unsigned char a = 0xff;

	for (i = 0; i < BLOCK_SIZE; i++) {

		if (inode_bm[i] != a) {

			for (int j = 1; j < 9; j++) {

				char c = (inode_bm[i] << j-1) & 0x80;
				if (c == 0x00) {

					inode_number = i * 8 + j - 1;
					inode_bm[i] |= int_pow(2, 7 - j + 1);
					sb.s_free_inodes_count--;
					fseek(fp, 0, SEEK_SET);
					fwrite(&sb, 28, 1, fp);
					for (i = 0; i < 121; i++) {

    					fwrite(&offset, 4, 1, fp);
   		 			}
					fwrite(block_bm, 512, 1, fp);
					fwrite(inode_bm, 512, 1, fp);
					return inode_number;
				}	 
			}
		}
	}

	return -1;
}

int get_block() {

	int block_number;
	int i;
	unsigned char a = 0xff;

	for (i = 0; i < BLOCK_SIZE; i++) {

		//printf("%x\n", block_bm[i]);

		if (block_bm[i] != a) {

			//printf("ace\n");

			for (int j = 1; j < 9; j++) {

				char c = (block_bm[i] << j-1) & 0x80;
				if (c == 0x00) {

					block_number = i * 8 + j - 1;
					block_bm[i] |= int_pow(2, 7 - j + 1);
					sb.s_free_blocks_count--;
					fseek(fp, 0, SEEK_SET);
					fwrite(&sb, 28, 1, fp);
					for (i = 0; i < 121; i++) {

				    	fwrite(&offset, 4, 1, fp);
				    }
					fwrite(block_bm, 512, 1, fp);
					fwrite(inode_bm, 512, 1, fp);
					return block_number;
				}	 
			}
		}
	}

	return -1;
}

void make_directory(char *path, unsigned int n)
{
    /*
     * For the file system image that is currently opened.
     * Make a new directory at the path provided.
     * Make sure that the path is valid.
     */

	if (sb.s_free_inodes_count == 0 || sb.s_free_blocks_count == 0) {

		printf("ACE error: no more space\n");
		return;
	}

    int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[i+1];
    char fil[n - i];
    
    //printf("ace\n");

    strncpy(dir, path, i);
    dir[i] = '\0';
    //printf("dir: %s\n", dir);
    
    strncpy(fil, path + i + 1, n - i);
    fil[n - i - 1] = '\0';
    //printf("file name: %s\n", fil)

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);
    printf("file: %s\n", fil);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    struct directory_entry dirent;

    int _x = 100;
    
	access_inode(parinode);

    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
    
    for (x = 0; x < 8; x++) {           

        fread(&dirent, 64, 1, fp);

        if (dirent.d_file_type == 0) {

        	if (_x > x)
        		_x = x;
            continue;
        }
        //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
        if (!strncmp(dirent.d_name, fil, 57)) {

            printf("ACE error: dir already exist\n");
            return;
        }
    }

    if (_x == 100) {

    	printf("ACE error: no more space1\n");
    	return;
    }

    ino.i_size += 64;

    fseek(fp, 3*512+parinode*64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    int filinode = get_inode();
    int filblock = get_block();

    //rewrite dir block
    fseek(fp, 8*512 + ino.i_block[0]*512 + _x*64, SEEK_SET);

    dirent.d_inode = filinode;
    dirent.d_file_type = 2;
    dirent.d_name_len = strlen(fil);
    strncpy(dirent.d_name, fil, 57);

    fwrite(&dirent, 64, 1, fp);

    ino.i_mode = umask((mode_t)(S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO));
    ino.i_uid = (uint16_t)getuid();
    ino.i_gid = (uint16_t)getgid();
    ino.i_links_count = 0;
    ino.i_size = 128;
    ino.i_time = (uint16_t)time(NULL);
    ino.i_ctime = (uint16_t)time(NULL);
    ino.i_mtime = (uint16_t)time(NULL);
    ino.i_dtime = 0;
    ino.i_blocks = 1;
    ino.i_block[0] = filblock;

    fseek(fp, 3 * 512 + filinode * 64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    fseek(fp, 8 * 512 + filblock * 512, SEEK_SET);

    dirent.d_inode = filinode;
    dirent.d_file_type = 2;
    dirent.d_name_len = 1;
    strncpy(dirent.d_name, ".\0", 57);

    fwrite(&dirent, 64, 1, fp);

    dirent.d_inode = parinode;
    dirent.d_file_type = 2;
    dirent.d_name_len = 2;
    strncpy(dirent.d_name, "..\0", 57);

    fwrite(&dirent, 64, 1, fp);
}

unsigned int read_directory(char *path, unsigned int n, char *data)
{
    /*
     * For the file system image that is currently opened.
     * Read the contents of the directory in the path provided.
     * Make sure that the path is valid.
     * Place the directory entries in data.
     * and return the number of bytes that have been placed in data.
     */

	char * buffer = malloc(512);
	char * res = buffer;
	int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[n+1];
    
    //printf("ace\n");

    strncpy(dir, path, n);
    dir[n] = '\0';
    //printf("dir: %s\n", dir);

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    access_inode(parinode);

    if (S_ISREG(ino.i_mode)) {

    	printf("ACE error: not a dir\n");
    	return;
    }

    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);

    struct directory_entry dirent;

    i = 0;

    for (x = 0; x < 8; x++) {                    

	    fread(&dirent, 64, 1, fp);
	   
	    if (dirent.d_file_type == 0) {
	    	//printf("found\n");
	    	continue;
	    }
	    strncpy(data, &(dirent.d_inode), 4);
	    data += 4;
	    strncpy(data, &(dirent.d_file_type), 2);
	    data += 2;
	    strncpy(data, &(dirent.d_name_len), 1);
	    data += 1;
	    strncpy(data, &(dirent.d_name), 57);
	    //printf("ace\n");
	    data += 57;
	    i += 64;
	}

	return i;
}

void rm_directory(char *path, unsigned int n)
{
    /*
     * For the file system image that is currently opened.
     * Delete the directory in the path provided.
     * Make sure that the directory doesn't have any files remaining
     */

    int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[i+1];
    char fil[n - i];
    
    //printf("ace\n");

    strncpy(dir, path, i);
    dir[i] = '\0';
    //printf("dir: %s\n", dir);
    
    strncpy(fil, path + i + 1, n - i);
    fil[n - i - 1] = '\0';
    //printf("file name: %s\n", fil)

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);
    printf("file: %s\n", fil);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    struct directory_entry dirent;
    struct inode ino1;
    int parinode1;
    int _x = 100;
    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);

    for (x = 0; x < 8; x++) {                    

        fread(&dirent, 64, 1, fp);
       
        if (dirent.d_file_type == 0) {
        	//printf("found\n");
        	continue;
        }
        //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
        if (!strncmp(dirent.d_name, fil, 57)) {

        	ino1 = ino;
        	parinode1 = parinode;
        	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
        	fread(&ino, 64, 1, fp);
        	//printf("ace: found\n");
            parinode = dirent.d_inode;
            if (S_ISREG(ino.i_mode)) {

            	printf("ACE error: not a dir\n");
            	return;
            }
            if (ino.i_size > 128) {

            	printf("ACE error: non-empty dir\n");
            	return;
            }
            _x = x;
        	break;
        }
    }

    if (x == 8 || _x == 100) {

        printf("ACE error: bad path\n");
        return;
    }

    ino1.i_size -= 64;

    fseek(fp, 3*512+parinode1*64, SEEK_SET);
    fwrite(&ino1, 64, 1, fp);

    fseek(fp, 8*512 + ino1.i_block[0]*512 + 64 * _x, SEEK_SET);
    for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    delete_block(ino.i_block[0]);
    delete_inode(parinode);
}

void create_file(char *path, unsigned int n, unsigned int size, char *data)
{
    /*
     * For the file system image that is currently opened.
     * Create a new file at path.
     * Make sure that the path is valid.
     * Write to the contents of the file in the data provided.
     * size is the number of bytes in the data.
     */

	if (size > 512 * 8) {

		printf("ACE error: file too large\n");
		return;
	}

	int n_blocks = (size / 512) + (size % 512 != 0);

	if (sb.s_free_inodes_count == 0 || sb.s_free_blocks_count < n_blocks) {

		printf("ACE error: no more space\n");
		return;
	}

    int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[i+1];
    char fil[n - i];
    
    //printf("ace\n");

    strncpy(dir, path, i);
    dir[i] = '\0';
    //printf("dir: %s\n", dir);
    
    strncpy(fil, path + i + 1, n - i);
    fil[n - i - 1] = '\0';
    //printf("file name: %s\n", fil)

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);
    printf("file: %s\n", fil);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    struct directory_entry dirent;

    int _x = 100;
    
	access_inode(parinode);

    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
    
    for (x = 0; x < 8; x++) {           

        fread(&dirent, 64, 1, fp);

        if (dirent.d_file_type == 0) {

        	if (_x > x)
        		_x = x;
            continue;
        }
        //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
        if (!strncmp(dirent.d_name, fil, 57)) {

            printf("ACE error: fil already exist\n");
            return;
        }
    }

    if (_x == 100) {

    	printf("ACE error: no more space\n");
    	return;
    }

    ino.i_size += 64;

    fseek(fp, 3*512+parinode*64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    int filinode = get_inode();

    //rewrite dir block
    fseek(fp, 8*512 + ino.i_block[0]*512 + _x*64, SEEK_SET);

    dirent.d_inode = filinode;
    dirent.d_file_type = 1;
    dirent.d_name_len = strlen(fil);
    strncpy(dirent.d_name, fil, 57);

    fwrite(&dirent, 64, 1, fp);

    ino.i_mode = umask((mode_t)(S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO));
    ino.i_uid = (uint16_t)getuid();
    ino.i_gid = (uint16_t)getgid();
    ino.i_links_count = 0;
    ino.i_size = size;
    ino.i_time = (uint16_t)time(NULL);
    ino.i_ctime = (uint16_t)time(NULL);
    ino.i_mtime = (uint16_t)time(NULL);
    ino.i_dtime = 0;
    ino.i_blocks = n_blocks;
    for (i = 0; i < n_blocks; i++) {

    	ino.i_block[i] = get_block();
    }

    fseek(fp, 3 * 512 + filinode * 64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    for (i = 0; i < n_blocks - 1; i++) {

    	fseek(fp, 8 * 512 + ino.i_block[i] * 512, SEEK_SET);
    	printf("%d\n", ino.i_block[i]);
    	for (j = 0; j < 512; j++) {

    		fwrite(data, 1, 1, fp);
    		data++;
    	}
    	size -= 512;
    }

    for (j = 0; j < size; j++) {

    		fwrite(data, 1, 1, fp);
    		data++;
    }
}

void rm_file(char *path, unsigned int n)
{
    /*
     * For the file system image that is currently opened.
     * Delete the file in the path provided.
     * Make sure that the data blocks are freed after deleting the file.
     */

	int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[i+1];
    char fil[n - i];
    
    //printf("ace\n");

    strncpy(dir, path, i);
    dir[i] = '\0';
    //printf("dir: %s\n", dir);
    
    strncpy(fil, path + i + 1, n - i);
    fil[n - i - 1] = '\0';
    //printf("file name: %s\n", fil)

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);
    printf("file: %s\n", fil);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    struct directory_entry dirent;
    struct inode ino1;
    int parinode1;
    int _x = 100;
    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);

    for (x = 0; x < 8; x++) {                    

        fread(&dirent, 64, 1, fp);
       
        if (dirent.d_file_type == 0) {
        	//printf("found\n");
        	continue;
        }
        //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
        if (!strncmp(dirent.d_name, fil, 57)) {

        	ino1 = ino;
        	parinode1 = parinode;
        	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
        	fread(&ino, 64, 1, fp);
        	//printf("ace: found\n");
            parinode = dirent.d_inode;
            if (S_ISDIR(ino.i_mode)) {

            	printf("ACE error: not a file\n");
            	return;
            }
            _x = x;
        	break;
        }
    }

    if (x == 8 || _x == 100) {

        printf("ACE error: bad path\n");
        return;
    }

    ino1.i_size -= 64;

    fseek(fp, 3*512+parinode1*64, SEEK_SET);
    fwrite(&ino1, 64, 1, fp);

    fseek(fp, 8*512 + ino1.i_block[0]*512 + 64 * _x, SEEK_SET);
    for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    for (i = 0; i < ino.i_blocks; i++) {

    	delete_block(ino.i_block[i]);
    }

    delete_inode(parinode);
}
unsigned int read_file(char *path, unsigned int n, char *data)
{
    /*
     * For the file system image that is currently opened.
     * Read the contents of the file in the path provided.
     * Make sure that the path is valid.
     * Place the file contents in data
     * and return the number of bytes in the file.
     */

	char * buffer = malloc(512);
	char * res = buffer;
	int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[n+1];
    
    //printf("ace\n");

    strncpy(dir, path, n);
    dir[n] = '\0';
    //printf("dir: %s\n", dir);

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    access_inode(parinode);

    if (S_ISDIR(ino.i_mode)) {

    	printf("ACE error: not a file\n");
    	return;
    }

    //printf("ace\n");

    int size = ino.i_size;

    for (i = 0; i < ino.i_blocks - 1; i++) {

    	fseek(fp, 8*512 + ino.i_block[i]*512, SEEK_SET);

    	for (j = 0; j < 512; j++) {

    		fread(data, 1, 1, fp);
    		data++;
    	}

    	size -= 512;
    }

    fseek(fp, 8*512 + ino.i_block[i]*512, SEEK_SET);

    for (j = 0; j < size; j++) {

    		fread(data, 1, 1, fp);
    		data++;
    }

    return size;
}
void make_link(char *path, unsigned int n, char *target)
{
    /*
     * make a new hard link in the path to target
     * make sure that the path and target are both valid.
     */

	int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("ACE error: bad path\n");
        return;
    }

    char dir[n+1];
    
    //printf("ace\n");

    strncpy(dir, path, n);
    dir[n] = '\0';
    //printf("dir: %s\n", dir);

    //if (fp == NULL) printf("ace\n");

    fseek(fp, 3*512 + 2*64, SEEK_SET);
    struct inode ino;
    fread(&ino, 64, 1, fp);

    //printf("%u\n", ino.i_blocks);

    char _dir[57];
    i = 0;

    printf("dir: %s\n", dir);

    //return;

    uint32_t parinode = 2;

    //locate inter dirs
    while (true) {

        j = 0;

        while (dir[i] != '/' && dir[i] != '\0') {

            _dir[j] = dir[i];
            i++;
            j++;
        }

        if (j == 0) {

            if (dir[i] == '/')
                i++;

            else if (dir[i] == '\0')
                break;
        }

        //look for the next inter dir name
        else {

            _dir[j] = '\0';
            j++;
            struct directory_entry dirent;

            //printf("%u\n", size);

            access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("ACE error: not a dir\n");
		    	return;
		    }

            fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);
            
            for (x = 0; x < 8; x++) {                    

                fread(&dirent, 64, 1, fp);
               
                if (dirent.d_file_type == 0) {
                	//printf("found\n");
                	continue;
                }
                //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
                if (!strncmp(dirent.d_name, _dir, 57)) {

                	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
                	fread(&ino, 64, 1, fp);
                	//printf("ace: found\n");
                    parinode = dirent.d_inode;
                	break;
                }
            }

            if (x == 8) {

                printf("ACE error: bad path\n");
                return;
            }
        }
    }

    struct directory_entry dirent;
    int _x = 100;
    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);

    for (x = 0; x < 8; x++) {                    

        fread(&dirent, 64, 1, fp);
       
        if (dirent.d_file_type == 0) {
        	//printf("found\n");
        	continue;
        }
        //printf("an entry: %u, %u, %u, %s\n", dirent.d_inode, dirent.d_file_type, dirent.d_name_len, dirent.d_name);
        if (!strncmp(dirent.d_name, target, 57)) {

        	fseek(fp, 3*512 + dirent.d_inode * 64, SEEK_SET);
        	fread(&ino, 64, 1, fp);
        	//printf("ace: found\n");
            parinode = dirent.d_inode;
            if (S_ISDIR(ino.i_mode)) {

            	printf("ACE error: not a file\n");
            	return;
            }
            _x = x;
        	break;
        }
    }

    if (x == 8 || _x == 100) {

        printf("ACE error: bad path\n");
        return;
    }

    ino.i_links_count++;

    fseek(fp, 3*512 + parinode * 64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);
}
