#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/gfp.h>
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
#define NUM_ENTRIES 512
//This file will contain the macros that will be used to call
//a custom syscall as if it was an 'embedded' one.
//Eg. sys_call_adder("syscall_name") will be called inside the kernel module
//that contains the new syscall to add. (TODO automate the process)
#define CUSTOM_SYSCALL_MACROS "~/custom_syscall_macros.h"
#define MACRO_SYS_start "#%d"
#define MACRO_SYS_end "#end"



MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("This module will create a syscall that will be used \
					to add new syscalls. A new header file will be created \
					and the header file has to be used BOTH in: \
					1) Module where the syscall to insert is defined; \
					2) User C file that will use that syscall.");


unsigned long sys_call_table_address = 0;
unsigned long sysnisyscall_addr = 0;

//used to avoid ononimous
char **syscall_names = {[0 ... NUM_ENTRIES] = 0};
int **syscall_cts_numbers = {[0 ... NUM_ENTRIES] = 0};
int total_syscall_added = 0;

void update_macro_file(void)
{
	//TODO
	printk(KERN_DEBUG "TODO create_header");
}


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

int update_syscalltable_entry(void* custom_syscall)
{
	int syst_entry;
	unsigned long cr0;

	syst_entry = find_syscalltable_free_entry();
	if(syst_entry == -1){
		printk(KERN_DEBUG "Free entry not found\n");
		return -1;
	}
	printk(KERN_DEBUG "%s: Index found: %d\n",MODNAME, syst_entry);

	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	((void **)sys_call_table_address)[syst_entry] = custom_syscall;
	write_cr0(cr0);

	return syst_entry;
}


asmlinkage int syscall_adder(void* new_syscall, char *syscall_name_user, int sysname_len, int cst_entry, int num_parameters)
{
	int ret;
	//this should be the max length of the macro, but i'm lazy, 1PG should be ok.
	int DIM = 4096;
	int SYSCALL_NAME_MAX_LEN = 1024;
	int customsys_free_indx;

	char *macro_line_0 = "#DEFINE %s() syscall(%d)";
	char *macro_line_1 = "#DEFINE %s(arg1) syscall(%d, arg1)";
	char *macro_line_2 = "#DEFINE %s(arg1, arg2) syscall(%d, arg1, arg2)";
	char *macro_line_3 = "#DEFINE %s(arg1, arg2, arg3) syscall(%d, arg1, arg2, arg3)";
	char *macro_line_4 = "#DEFINE %s(arg1, arg2, arg3, arg4) syscall(%d, arg1, arg2, arg3, arg4)";
	char *macro_line_5 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5) syscall(%d, arg1, arg2, arg3, arg4, arg5)";
	char *macro_line_6 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5, arg6) syscall(%d, arg1, arg2, arg3, arg4, arg5, arg6)";
	char *macro_line_used = NULL;
	char *syscall_name = NULL;

	
	if(try_module_get(THIS_MODULE) == 0){
		printk(KERN_ERR "%s: Module in use, try later\n", MODNAME);
		return -1;
	}
	

	if((syscall_name = kmalloc(SYSCALL_NAME_MAX_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc syscall_name\n", MODNAME);

		module_put(THIS_MODULE);
		return -1;
	}

	if(copy_from_user(syscall_name, syscall_name_user, sysname_len) != 0){
		printk(KERN_ERR "%s: Unable to strncopy_from_user syscall_name\n", MODNAME);

		module_put(THIS_MODULE);
		return -1;
	}
	syscall_name[sysname_len] = '\0';

	if((macro_line_used = kmalloc(DIM, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc\n", MODNAME);

		module_put(THIS_MODULE);
		kfree(syscall_name);
		return -1;
	}

	switch (num_parameters){
		case 0:
			ret = snprintf(macro_line_used, DIM,  macro_line_0, syscall_name, cst_entry);
			break;
		case 1:
			ret = snprintf(macro_line_used, DIM, macro_line_1, syscall_name, cst_entry);
			break;
		case 2:
			ret = snprintf(macro_line_used, DIM, macro_line_2, syscall_name, cst_entry);
			break;
		case 3:
			ret = snprintf(macro_line_used, DIM, macro_line_3, syscall_name, cst_entry);
			break;
		case 4:
			ret = snprintf(macro_line_used, DIM, macro_line_4, syscall_name, cst_entry);
			break;
		case 5:
			ret = snprintf(macro_line_used, DIM, macro_line_5, syscall_name, cst_entry);
			break;
		case 6:
			ret = snprintf(macro_line_used, DIM, macro_line_6, syscall_name, cst_entry);
			break;
		default:
			printk(KERN_ERR "%s: Invalid number parameter (%d)\n", MODNAME, num_parameters);
			
			kfree(macro_line_used);
			kfree(syscall_name);
			module_put(THIS_MODULE);
			return -1;
	}

	if(ret <= 0){
		printk(KERN_ERR "%s: snprintf failed, returning (%d)\n", MODNAME, ret);
		
		kfree(macro_line_used);
		kfree(syscall_name);
		module_put(THIS_MODULE);
		return -1;
	}

	//mutex lock

	//adding entry in system call table
	if((customsys_free_indx = update_syscalltable_entry(new_syscall)) == -1){
		printk(KERN_ERR "%s: Free entry not found while adding %s\n", MODNAME, syscall_name);
		
		kfree(macro_line_used);
		kfree(syscall_name);
		module_put(THIS_MODULE);
		return -1;
	}

	//inserting the line in the macro file

	//update local arrays
	if((syscall_names[total_syscall_added] = kmalloc(SYSCALL_NAME_MAX_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc temp\n", MODNAME);

		module_put(THIS_MODULE);
		kfree(syscall_name);
		kfree(macro_line_used);
		return -1;
	}
	//TODO memocpy -> syscall_names[total_syscall_added] to syscall_name
	syscall_cts_numbers[total_syscall_added] = customsys_free_indx;

	//mutex unlock

	total_syscall_added++;
	kfree(macro_line_used);
	kfree(syscall_name);
	module_put(THIS_MODULE);
	return 0;
}

asmlinkage int syscall_remover(void/*int cst_entry*/)
{
	//module get
	//TODO
	printk(KERN_DEBUG "TODO remover");

	//module put
	return 0;
}

static int __init install(void)
{

	//TODO create the header file, add the adder entry.

	//when a new syscall has to be added, the header created has to be included,
	//then call syscall(NR, void* syscall_toadd_definedinmodule), and the new
	//entry will be inserted.
	

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
	if(update_syscalltable_entry(syscall_adder) == -1){
		printk(KERN_DEBUG "%s: Syscall_adder not inserted\n", MODNAME);
		return -1;
	}

	//adding sys remover
	if(update_syscalltable_entry(syscall_remover) == -1){
		printk(KERN_DEBUG "%s: Syscall_remover not inserted\n", MODNAME);
		return -1;
	}

	//creating file header

	printk(KERN_DEBUG "%s: syscall_adder & syscall_remover added correctly\n", MODNAME);

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
