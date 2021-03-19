#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "/home/ezio/custom_syscall_macros.h"

int main(int argc, char **argv)
{

	int ret;
	ret = your_syscall();
	printf("Syscall returned %d\n", ret);

	return 0;
}