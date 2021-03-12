#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
	if(argc != 2){
		printf("Usage ./test <syscall_number>\n");
		return -1;
	}
	
	int syscall_num = atoi(argv[1]);
	int ret;
	ret = syscall(syscall_num);
	printf("Syscall %d returned %d\n", syscall_num, ret);

	return 0;
}