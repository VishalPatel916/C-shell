#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    char buf[100];
    int fd;

    fd = open("README", O_RDONLY);
    if(fd < 0){
        printf("Error opening file\n");
        exit(1);
    }

    printf("Initial read count: %ld\n", getreadcount());

    read(fd, buf, 100);

    printf("Read count after reading 100 bytes: %ld\n", getreadcount());

    close(fd);
    exit(0);
}
