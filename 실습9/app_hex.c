#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

int main(void){
  int dev, data, rdata;

  dev = open("/dev/hex", O_RDWR);
  if(dev<0){
    fprintf(stderr, "Cannot Open LED device.");
    return 1;
  }

  //write
  printf("Input HEX7 data (hex) : ");
  scanf("%x", &data);
  write(dev, &data, 4);

  //read
  read(dev, &rdata, 4);
  printf("read data : %x \n", rdata);

  return 0;
}