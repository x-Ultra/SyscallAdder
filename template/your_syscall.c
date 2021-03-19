#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("What a cool description !");

#define MODNAME "Your cool module"

extern int syscall_remover(int syscall_indx);
extern int syscall_adder(void* syscall_addr, char* syscall_name, int syscall_name_length, int syscall_num_params);

//maintain the indexes returned by syscall_adder, in order to 
//remove them, if needed.
int your_syscall_indx, your_syscall_args_indx;

asmlinkage void your_syscall(void)
{
	printk(KERN_DEBUG "I am a syscall with 0 param!");
}

asmlinkage void your_syscall_args(int pippo)
{
	printk(KERN_DEBUG "I am a syscall with 1 param: %d", pippo);
}
// .
// .
// .
/*
	and so on up to 6 parameter
*/


static int __init install(void)
{


	printk(KERN_DEBUG "%s: Adding %s\n", MODNAME, "your_syscall");
	if((your_syscall_indx = syscall_adder(your_syscall, "your_syscall", 12, 0)) == -1){
		printk("%s: Unable to add 0 param func\n", MODNAME);
	}

	printk(KERN_DEBUG "%s: Adding %s\n", MODNAME, "your_syscall_args");
	if((your_syscall_args_indx = syscall_adder(your_syscall, "your_syscall_args", 17, 1)) == -1){
		printk("%s: Unable to add 1 param func\n", MODNAME);
	}
	
	printk(KERN_DEBUG "%s: Syscall succesfuly added !\n", MODNAME);

	return 0;
}

static void __exit uninstall(void)
{

	int ret = 0;
	if(syscall_remover(your_syscall_indx) == -1){
		printk("%s: Unable to remove 0 param syscall\n", MODNAME);
		ret = -1;
	}

	if(syscall_remover(your_syscall_args_indx) == -1){
		printk("%s: Unable to remove 1  param syscall\n", MODNAME);
		ret = -1;
	}

	if(ret != -1){
		printk(KERN_DEBUG "%s: Systemcalls removed correctly !\n", MODNAME);
	}

}

module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");