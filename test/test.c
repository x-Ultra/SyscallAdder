#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{

	int ret, syscall_num;

	if(argc != 6){
		printf("Usage ./test <Syscall_adder_num> <string> <syscall_name> <entry_num> <num_param>\n");
		return -1;
	}

	syscall_num = atoi(argv[1]);
	ret = syscall(syscall_num, (void *)argv[2], argv[3], strlen(argv[3]), atoi(argv[4]), atoi(argv[5]));
	
	printf("Syscall_adder %d returned %d\n", syscall_num, ret);

	return 0;
}