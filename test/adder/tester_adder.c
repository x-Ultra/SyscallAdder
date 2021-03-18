#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("Tester module");

#define MODNAME "tester_adder"

extern int syscall_adder(void* a, char* b, int c, int d);

asmlinkage void syscall_test(void)
{
	printk(KERN_DEBUG "syscall_test called !");
}

asmlinkage void syscall_test_args(int pippo)
{
	printk(KERN_DEBUG "syscall_test_args called, param: %d", pippo);
}

static int __init install(void)
{

	if(syscall_adder(syscall_test, "syscall_test", 12, 0) == -1){
		printk("%s: Unable to add 0 param func\n", MODNAME);
	}

	if(syscall_adder(syscall_test_args, "syscall_test_args", 17, 1) == -1){
		printk("%s: Unable to add 1 param func\n", MODNAME);
	}
	
	printk(KERN_DEBUG "%s: Tester modeule done !\n", MODNAME);

	return 0;
}

static void __exit uninstall(void)
{
	//1. replace installed syscall in table, with sysni_syscall
	//2. delete header

	printk("No operations needed to unload the module\n");

}

module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");