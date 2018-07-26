/*
  FUSE: Filesystem in Userspace
  This skeleton code is made from the function prototypes found in
  /usr/include/fuse/fuse.h Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPL v2.
*/

#include "fusefs.h"

int offset = 0;

/**
 *--------------
 *    really
 *------------
 */
int int_pow(int a, int b) 
{

	int res = 1;

	for (int i = 0; i < b; i++) {

		res *= a;
	}

	return res;
}

void access_inode(uint32_t inode_number) 
{

	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	ino.i_time = (uint32_t)time(NULL);
	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	fwrite(&ino, 64, 1, fp);
}

void modify_inode(uint32_t inode_number) 
{

	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	ino.i_mtime = (uint32_t)time(NULL);
	fseek(fp, 3 * 512 + inode_number * 64, SEEK_SET);
	fwrite(&ino, 64, 1, fp);
}

void delete_inode(uint32_t inode_number) 
{

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
	fseek(fp, 3*512 + 64*inode_number, SEEK_SET);
	for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
}

void delete_block(uint32_t block_number) 
{

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
	fseek(fp, 8*512 + 512*block_number, SEEK_SET);
	for (i = 0; i < 128; i++) {

    	fwrite(&offset, 4, 1, fp);
    }
}

int get_inode() 
{

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

int get_block() 
{

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

uint32_t path_to_inode(const char *path, unsigned int n) 
{

	int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("\n\n\nACE error: bad path in translation\n");
        printf("%s\n\n\n", path);
        errno = ENOENT;
        return -1;
    }

    char dir[n + 1];
    
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

    //printf("dir: %s\n", dir);

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

            //access_inode(parinode);

            if (S_ISREG(ino.i_mode)) {

		    	printf("\n\n\nACE error: not a dir in translation\n");
		    	printf("%s\n\n\n", _dir);
		    	errno = ENOENT;
		    	return -1;
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

                printf("\n\n\nACE error: file not exist in translation\n");
                printf("%s\n\n\n", path);
                errno = ENOENT;
                return -1;
            }
        }
    }

    return parinode;
}

static void *sfs_mount(struct fuse_conn_info *conn) 
{

	open_filesystem("simpleFS\0", 8);
}

static void sfs_unmount (void *private_data) 
{

	fclose(fp);
}


 static int sfs_getattr(const char *path, struct stat *stbuf)
{
	
	printf("\n\n\nACE getattr: %s\n\n\n", path);

	uint32_t number_inode = path_to_inode(path, strlen(path));

	if (number_inode == -1) {

		printf("\n\n\ngetattr bad number_inode\n\n\n");
		errno = ENOENT;
		return -errno;
	}

	fseek(fp, 3*512 + number_inode*64, SEEK_SET);
	struct inode ino;
	fread(&ino, 64, 1, fp);
	stbuf->st_mode = ino.i_mode;
	stbuf->st_nlink = ino.i_links_count;
	stbuf->st_uid = ino.i_uid;
	stbuf->st_gid = ino.i_gid;
	stbuf->st_size = ino.i_size;
	stbuf->st_atime = ino.i_time;
	stbuf->st_mtime = ino.i_mtime;
	stbuf->st_ctime = ino.i_ctime;
	stbuf->st_blocks = ino.i_blocks;

	return 0;
}

static int sfs_mkdir(const char *path, mode_t mode)
{
	
	printf("\n\n\nACE mkdir: %s\n\n\n", path);
	

	if (sb.s_free_inodes_count == 0 || sb.s_free_blocks_count == 0) {

		printf("\n\n\nACE error: no more space in mkdir\n");
		printf("%s\n\n\n", path);
		errno = ENOMEM;
		return 0 - errno;
	}

	int n = strlen(path);

    int i;
    int j;
    int k;
    int x;
    int grace = 0;

    for (i = n; i >= 0; i--) {

        if (path[i] == '/') break;
    }
    
    if (i < 0) {

        printf("\n\n\nACE error: bad path in mkdir\n");
        printf("%s\n\n\n", path);
        errno = ENOENT;
        return 0 - errno;
    }

    char dir[i + 1];
    char fil[n - i];
    
    //printf("ace\n");

    if (i == 0) {
    	strcpy(dir, "/");
    }
    else {
    	strncpy(dir, path, i);
    	dir[i] = '\0';
	}
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

    printf("\n\n\nmkdir: dir: %s\n", dir);
    printf("\n\n\nmkdir: file: %s\n\n\n", fil);

    uint32_t number_inode = path_to_inode(dir, strlen(dir));

    if (number_inode == -1) {

		printf("\n\n\nmkdir bad number_inode\n\n\n");
		errno = ENOENT;
		return 0 - errno;
	}

    make_dir_node(number_inode, fil);
    return 0;
}


static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{

	uint32_t number_inode = path_to_inode(path, strlen(path));

	if (number_inode == -1) {

		printf("\n\n\nreaddir bad number_inode\n\n\n");
		errno = ENOENT;
		return 0 - errno;
	}

	printf("\n\n\nACE read dir: %s\n\n\n", path);

	struct inode ino;

	fseek(fp, 3*512 + number_inode * 64, SEEK_SET);
    fread(&ino, 64, 1, fp);
	
    fseek(fp, 8*512 + ino.i_block[0]*512, SEEK_SET);

	struct directory_entry dirent;

	int x;


    for (x = 0; x < 8; x++) {                    

	    fread(&dirent, 64, 1, fp);
	   

	    if (dirent.d_file_type == 0) {
	    	//printf("found\n");
	    	continue;
	    }
	    printf("%s\n", dirent.d_name);
	    struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = dirent.d_inode;
		st.st_mode = dirent.d_file_type;
		if (filler(buf, dirent.d_name, &st, 0))
			break;
	}

	return 0;
}


static int sfs_create(const char *path, mode_t mode, dev_t rdev)
{

	int n = strlen(path);

	int n_blocks = 0;

	if (sb.s_free_inodes_count == 0 || sb.s_free_blocks_count < n_blocks) {

		printf("ACE error: no more space\n");
		errno = ENOMEM;
		return -errno;
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
        errno = ENOENT;
        return -errno;
    }

    char dir[i + 1];
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

    printf("\n\n\nACE create: dir: %s\n", dir);
    printf("ACE create: file: %s\n\n\n", fil);

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
		    	errno = ENOENT;
		    	return -errno;
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
                errno = ENOENT;
		    	return -errno;
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
            errno = ENOENT;
		    return -errno;
        }
    }

    if (_x == 100) {

    	printf("ACE error: no more space\n");
    	errno = ENOMEM;
		return -errno;
    }

    ino.i_size += 64;

    fseek(fp, 3*512 + parinode*64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    //rewrite dir block
    int temp = ino.i_block[0];

    ino.i_mode = umask((mode_t)(S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO));
    ino.i_uid = (uint16_t)getuid();
    ino.i_gid = (uint16_t)getgid();
    ino.i_links_count = 0;
    ino.i_size = 0;
    ino.i_time = (uint16_t)time(NULL);
    ino.i_ctime = (uint16_t)time(NULL);
    ino.i_mtime = (uint16_t)time(NULL);
    ino.i_dtime = 0;
    ino.i_blocks = n_blocks;
    for (i = 0; i < n_blocks; i++) {

    	ino.i_block[i] = get_block();
    }

    int filinode = create_inode(&ino);

    dirent.d_inode = filinode;
    dirent.d_file_type = 1;
    dirent.d_name_len = strlen(fil);
    strncpy(dirent.d_name, fil, 57);

    fseek(fp, 8*512 + temp*512 + _x*64, SEEK_SET);

    fwrite(&dirent, 64, 1, fp);

    fseek(fp, 3 * 512 + filinode * 64, SEEK_SET);
    fwrite(&ino, 64, 1, fp);

    printf("\n\n\nACE create at inode: %d\n\n\n", filinode);

    return 0;
}


static int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{

	char *data = buf;
	char * buffer = malloc(512);
	char * res = buffer;
	int n = strlen(path);
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
        errno = ENOENT;
        return -errno;
    }

    char dir[n + 1];
    
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
		    	errno = ENOENT;
		    	return -errno;
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
                errno = ENOENT;
		    	return -errno;
            }
        }
    }

    access_inode(parinode);

    if (S_ISDIR(ino.i_mode)) {

    	printf("ACE error: not a file\n");
    	errno = ENOSYS;
		return -errno;
    }

    //printf("ace\n");

    int _size = ino.i_size;

    for (i = 0; i < ino.i_blocks - 1; i++) {

    	fseek(fp, 8*512 + ino.i_block[i]*512, SEEK_SET);

    	for (j = 0; j < 512; j++) {

    		fread(data, 1, 1, fp);
    		data++;
    	}

    	_size -= 512;
    }

    fseek(fp, 8*512 + ino.i_block[i]*512, SEEK_SET);

    for (j = 0; j < _size; j++) {

    		fread(data, 1, 1, fp);
    		data++;
    }

    return _size;
}


static int sfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{

}

static int sfs_remove_dir(const char *path) 
{

	int n = strlen(path);

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
        errno = ENOENT;
        return -errno;
    }

    char dir[i + 1];
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
		    	errno = ENOENT;
        		return -errno;
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
                errno = ENOENT;
        		return -errno;
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
            	errno = ENOENT;
        		return -errno;
            }
            if (ino.i_size > 128) {

            	printf("ACE error: non-empty dir\n");
            	errno = ENOSYS;
            	return -errno;
            }
            _x = x;
        	break;
        }
    }

    if (x == 8 || _x == 100) {

        printf("ACE error: bad path\n");
        errno = ENOENT;
        return -errno;
    }

    ino1.i_size -= 64;

    fseek(fp, 3*512 + parinode1*64, SEEK_SET);
    fwrite(&ino1, 64, 1, fp);

    fseek(fp, 8*512 + ino1.i_block[0]*512 + 64 * _x, SEEK_SET);
    for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    delete_block(ino.i_block[0]);
    delete_inode(parinode);

    return 0;
}

static int sfs_delete(const char *path) 
{

	int n = strlen(path);
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
        errno = ENOENT;
        return -errno;
    }

    char dir[i + 1];
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
		    	errno = ENOENT;
        		return -errno;
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
                errno = ENOENT;
        		return -errno;
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
            	errno = ENOENT;
        		return -errno;
            }
            _x = x;
        	break;
        }
    }

    if (x == 8 || _x == 100) {

        printf("ACE error: bad path\n");
        errno = ENOENT;
        return -errno;
    }

    ino1.i_size -= 64;

    fseek(fp, 3*512 + parinode1*64, SEEK_SET);
    fwrite(&ino1, 64, 1, fp);

    fseek(fp, 8*512 + ino1.i_block[0]*512 + 64 * _x, SEEK_SET);
    for (i = 0; i < 16; i++) {

    	fwrite(&offset, 4, 1, fp);
    }

    for (i = 0; i < ino.i_blocks; i++) {

    	delete_block(ino.i_block[i]);
    }

    delete_inode(parinode);

    return 0;
}

static int sfs_symlink(const char *from, const char *to) 
{
}

static int sfs_readlink(const char *path, char *buf, size_t size)
{
}



static struct fuse_operations sfs_oper = 
{
    .init    = sfs_mount,
    .destroy = sfs_unmount,
    .getattr = sfs_getattr,
    .mkdir	 = sfs_mkdir,
    .readdir = sfs_readdir,
    .symlink = sfs_symlink,
    .readlink = sfs_readlink,
    .mknod	 = sfs_create,
    .read	 = sfs_read,
    .write	 = sfs_write,
    .unlink	 = sfs_delete,
    .rmdir       = sfs_remove_dir,
};

int main(int argc, char *argv[])
{
    umask(0); 
    return fuse_main(argc, argv, &sfs_oper, NULL);
}

