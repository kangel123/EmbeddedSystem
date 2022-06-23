#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define NOFILL 4
#define BLINK 8

int main(){
	int dev, data, number, rdata;
	
	dev = open("/dev/calculator", O_RDONLY);
	if (dev < 0){
		fprintf(stderr, "cannot open Caclulator device\n");
		return -1;
	}
	ioctl(dev, NOFILL, NULL);
	printf("Input one number : ");
	scanf("%d", &data);
	
	while(1){
		printf("\nSelect Option \n");
		printf("key0 : addition	key1 : subtraction 	key2 : multiplication	key3 :  division \n");
		
		read(dev, &rdata, 4);
		
		printf("Input another number : ");
		scanf("%d", &number);
		
		switch(rdata){
			case 1 :
				printf("%d + %d = ",data, number);
				data += number;
				printf("%d\n",data);
				break;
			case 2 :
				if(data < number){
					printf("ERROR : result is negative \n");
					data = -1;
				}
				else{
					printf("%d - %d = ",data, number);
					data -= number;
					printf("%d\n",data);
				}
				break;
			case 4 :
				printf("%d * %d = ",data, number);
				data *= number;
				printf("%d\n",data);
				break;
			case 8 :
				if(number == 0){
					printf("ERROR : not divisible by zero\n");
					data = -1;
				}
				else{
					printf("%d / %d = ",data, number);
					data /= number;
					printf("%d\n",data);
				}
				break;
			default :
				printf("ERROR\n");
				data = -1;
		}
		
		if(data >= 1000000){
			printf("ERROR : out of range of expression\n");
			break;
		}
		else if(0 > data){
			break;
		}
		else{
			write(dev, &data, 4);
		}
	}
	return 0;
}
