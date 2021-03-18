#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("Tester module");

#define MODNAME "tester_remover"

extern int syscall_remover(int);

asmlinkage void syscall_test_args(int pippo)
{
	printk(KERN_DEBUG "syscall_test called, param: %d", pippo);
}

static int __init install(void)
{

	if(syscall_remover(343) == -1){
		printk("%s: Unable to remove 0 param func\n", MODNAME);
	}

	if(syscall_remover(344) == -1){
		printk("%s: Unable to remove 1  param func\n", MODNAME);
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