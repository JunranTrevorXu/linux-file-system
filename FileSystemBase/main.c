#include "simpleFS.h"
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    /* TODO:
     * Implement :)
     */
	init_filesystem(20, "fs\0", 2);
	int fd = open("fs", O_RDONLY);

	struct stat buf;
    fstat(fd, &buf);
    int size = buf.st_size;

    printf("file system size: %d\n", size);

    //pg2 make dirs
    /*
    make_directory("/a", 2);
    make_directory("/b", 2);
    
    
    char * data = malloc(512);
    read_directory("/", 1, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
    make_directory("/a/b", 4);
    make_directory("/a/c", 4);
    data = malloc(512);
    read_directory("/a", 2, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
    make_directory("/c", 2);
    make_directory("/d", 2);
    make_directory("/e", 2);
    make_directory("/f", 2);
    data = malloc(512);
    read_directory("/", 1, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
	*/

    /* rm 1

    make_directory("/a", 2);
    rm_directory("/a", 2);
    char * data = malloc(512);
    read_directory("/", 1, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
    make_directory("/a", 2);
    make_directory("/a/b", 4);
    rm_directory("/a/b", 4);
    data = malloc(512);
    read_directory("/a", 2, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
    make_directory("/b", 2);
    make_directory("/c", 2);
    rm_directory("/b", 2);
    data = malloc(512);
    read_directory("/", 1, data);
    for (int i = 0; i < 512; i++) {

    	printf("%x ", *data);
    	if (i % 16 == 15) printf("\n");
    	data++;
    }
    */

    /* rm 2

    make_directory("/a", 2);
    rm_directory("/b", 2);
    */

    /* file 1

    char * data = "abababababababab\0";
    create_file("/A", 2, 17, data);
    rm_file("/A", 2);
    data = "abababababababab\0";
    create_file("/A", 2, 17, data);
    data = malloc(512 * 7);
    for (int i = 0; i < 512 * 7; i++) {

    	*data = 0xff;
    	data++;
    }
    create_file("/B", 2, 512 * 7, data);
    data = malloc(512);
    read_directory("/a", 2, data);
    

    char * data = "abababababababab\0";
    create_file("/A", 2, 17, data);
    create_file("/B", 2, 17, data);
    rm_file("/A", 2);
    */

    return 0;
}
