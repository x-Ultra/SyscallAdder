#include <linux/module.h>
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

//TODO
#define NUM_ENTRIES 512

//TODO edit home

//The header file in which the entry will be added
#define CUSTOM_SYSCALL_TAB "%s/custom_syscall_indx.h"

//This file will contain the macros that will be used to call
//a custom syscall as if it was an 'embedded' one.
//Eg. sys_call_adder("syscall_name") will be called inside the kernel module
//that contains the new syscall to add. (TODO automate the process)
#define CUSTOM_SYSCALL_MACROS "%s/custom_syscall_macros.h"

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
int **syscall_cts_number = NULL;


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

	return 0;
}


asmlinkage int syscall_adder(void* new_syscall, char *syscall_name, char *cst_entry, int num_parameters)
{

	//MOD_INC_USE_COUNT;

	char *macro_line_0 = "#DEFINE %s() syscall(%d)";
	char *macro_line_1 = "#DEFINE %s(arg1) syscall(%d, arg1)";
	char *macro_line_2 = "#DEFINE %s(arg1, arg2) syscall(%d, arg1, arg2)";
	char *macro_line_3 = "#DEFINE %s(arg1, arg2, arg3) syscall(%d, arg1, arg2, arg3)";
	char *macro_line_4 = "#DEFINE %s(arg1, arg2, arg3, arg4) syscall(%d, arg1, arg2, arg3, arg4)";
	char *macro_line_5 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5) syscall(%d, arg1, arg2, arg3, arg4, arg5)";
	char *macro_line_6 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5, arg6) syscall(%d, arg1, arg2, arg3, arg4, arg5, arg6)";

	char *macro_line_used = NULL;
	if((macro_line_used = kmalloc(PAGE_SIZE, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc\n", MODNAME);

		//MOD_DEC_USE_COUNT;
		return -1;
	}

	int ret;

	switch (num_parameters){
		case 0:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_0, syscall_name, cst_entry);
			break;
		case 1:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_1, syscall_name, cst_entry);
			break;
		case 2:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_2, syscall_name, cst_entry);
			break;
		case 3:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_3, syscall_name, cst_entry);
			break;
		case 4:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_4, syscall_name, cst_entry);
			break;
		case 5:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_5, syscall_name, cst_entry);
			break;
		case 6:
			ret = vsnprinf(macro_line_used, PAGE_SIZE, macro_line_6, syscall_name, cst_entry);
			break;
		default:
			prink(KERN_ERR "%s: Invalid number parameter (%d)\n", MODNAME, num_parameters);
			return -1;
	}

	if(ret <= 0){
		prink(KERN_ERR "%s: snprintf failed, returning (%d)\n", MODNAME, ret);
		
		//MOD_DEC_USE_COUNT;
		return -1;
	}

	printk(KERN_DEBUG "%s-> kmalloc+vsnprintf: %s\n", MODNAME, macro_line_used);

	//mutex lock

	//inserting the line in the macro file

	//update local arrays

	//mutex unlock

	kfree(macro_line_used);
	//MOD_DEC_USE_COUNT;

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
