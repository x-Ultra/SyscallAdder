#include <linux/module.h>
#include <linux/kern_levels.h>
#include <linux/kallsyms.h>
//stuff for reading/wrinting on file (via VFS)
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
//read_cr0
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif


#define MODNAME "syscall_adder"

//TODO
#define NUM_ENTRIES 512

//The header file in which the entry will be added
#define CUSTOM_SYSCALL_TAB "/home/ezio/custom_syscall_indx.h"

//This file will contain the macros that will be used to call
//a custom syscall as if it was an 'embedded' one.
//Eg. sys_call_adder("syscall_name") will be called inside the kernel module
//that contains the new syscall to add. (TODO automate the process)
#define CUSTOM_SYSCALL_MACROS "/home/ezio/custom_syscall_macros.h"

#define CST_sysc_adder "#define CST_syscal_adder %d"
#define CST_sysc_remover "#define CST_syscal_remover %d"

#define CST_ENTRY "#define CST_%s %d\n"



MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("This module will create a syscall that will be used \
					to add new syscalls. A new header file will be created \
					and the header file has to be used BOTH in: \
					1) Module where the syscall to insert is defined; \
					2) User C file that will use that syscall.");


unsigned long sys_call_table_address = 0;
unsigned long sysnisyscall_addr = 0;

//used to avoid ononimous
char **syscall_names = NULL;


int find_syscalltable_free_entry(void)
{
	void **temp_addr = (void **)sys_call_table_address;
	int i;

	for(i = 0; i < NUM_ENTRIES; i += 1){

		if(temp_addr[i] == (void *)sysnisyscall_addr){
			printk(KERN_DEBUG "Found free entry NR.%d\n", i);
			break;
		}
	}

	return i;
}


asmlinkage int syscall_adder(void/*void* new_syscall, char *est_entry*/)
{
	//TODO
	printk(KERN_DEBUG "TODO adder");
	return 4;
}

asmlinkage int syscall_remover(void/*int est_entry*/)
{
	//TODO
	printk(KERN_DEBUG "TODO remover");
	return 7;
}

void create_header(void)
{
	//TODO
	printk(KERN_DEBUG "TODO create_header");
}




static int __init install(void)
{
	//TODO create the header file, add the adder entry.

	//when a new syscall has to be added, the header created has to be included,
	//then call syscall(NR, void* syscall_toadd_definedinmodule), and the new
	//entry will be inserted.
	int sys_add_entry, sys_rem_entry;
	unsigned long cr0;

	sys_call_table_address = kallsyms_lookup_name("sys_call_table");

	if(sys_call_table_address == 0){
		printk(KERN_ERR "The syscalltable was NOT found, module NOT added\n");
		return -1;
	}

	sysnisyscall_addr = kallsyms_lookup_name("sys_ni_syscall");

	if(sysnisyscall_addr == 0){
		printk(KERN_ERR "TThe sysnisyscall was NOT found, module NOT added\n");
		return -1;
	}

	//adding sys adder
	sys_add_entry = find_syscalltable_free_entry();
	if(sys_add_entry == -1){
		printk(KERN_DEBUG "Free entry not found\n");
		return -1;
	}
	printk(KERN_DEBUG "%s: Index found: %d\n",MODNAME, sys_add_entry);

	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	((void **)sys_call_table_address)[sys_add_entry] = syscall_adder;
	write_cr0(cr0);

	//adding sys remover
	sys_rem_entry = find_syscalltable_free_entry();
	if(sys_add_entry == -1){
		printk(KERN_DEBUG "Free entry not found\n");
		return -1;
	}
	printk(KERN_DEBUG "%s: Index found: %d\n",MODNAME, sys_rem_entry);

	if(sys_rem_entry == sys_add_entry){
		printk(KERN_ERR "%s: The syscall table has NOT been updated\n", MODNAME);
		return -1;
	}

	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	((void **)sys_call_table_address)[sys_rem_entry] = syscall_remover;
	write_cr0(cr0);

	//creating file header
	create_header();

	printk(KERN_DEBUG "%s: syscall_adder & syscall_removeradded correctly\n", MODNAME);

	return 0;
}

static void __exit uninstall(void)
{
	//1. access the header
	//2. extract the syscall adder and remover
	//3. update the sys_call_table

	printk("No operations needed to unload the module\n");

}

module_init(install)
module_exit(uninstall)
MODULE_LICENSE("GPL");
