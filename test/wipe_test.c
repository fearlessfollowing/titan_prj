#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>


#ifndef BLKDISCARD
#define BLKDISCARD _IO(0x12,119)
#endif

#ifndef BLKSECDISCARD
#define BLKSECDISCARD _IO(0x12,125)
#endif


unsigned int get_blkdev_size(int fd)
{
  unsigned int nr_sec;

  if ((ioctl(fd, BLKGETSIZE, &nr_sec)) == -1) {
    nr_sec = 0;
  }

  return nr_sec;
}



int main(int argc, char* argv[])
{
    int fd;
    unsigned long long range[2];
    unsigned int numSectors = 0;
    
    if (argc < 2) {
        printf("Usage: wipe_test <path> \n");
        return -1;
    }

    printf("path = %s\n", argv[1]);

    fd = open(argv[1], O_RDWR);
    if (fd >= 0) {
        if (numSectors == 0) {
            numSectors = get_blkdev_size(fd);
        }

        if (numSectors == 0) {
            printf("Fat wipe failed to determine size of %s\n", argv[1]);
            close(fd);
            return -1;
        }

        range[0] = 0;
        range[1] = (unsigned long long)numSectors * 512;
        
        if (ioctl(fd, BLKDISCARD, &range) < 0) {
            printf("Fat wipe failed to discard blocks on %s \n", argv[1]);
        } else {
            printf("Fat wipe %d sectors on %s succeeded \n", numSectors, argv[1]);
        }
        close(fd);
    } else {
        printf("Fat wipe failed to open device %s\n", argv[1]);
    }
    return 0;
}