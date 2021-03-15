#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/kallsyms.h>
//rename & delete file
#include <linux/syscalls.h>
//kmalloc declaration
#include <linux/slab.h>
//header  for kmalloc flags
#include <linux/gfp.h>
//mutex
#include <linux/mutex.h>
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
#define MACRO_DIR "/home/ezio"
#define TEMP_MACROS_FILE_RAW "%s/custom_syscall_macros_temp.h"
#define CUSTOM_SYSCALL_MACROS_RAW "%s/custom_syscall_macros.h"
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
DEFINE_MUTEX(mod_mutex);

char CUSTOM_SYSCALL_MACROS[512];
char TEMP_MACROS_FILE[512];
char *syscall_names[NUM_ENTRIES] = { [ 0 ... NUM_ENTRIES-1 ] = 0 };
int syscall_cts_numbers[NUM_ENTRIES] = { [ 0 ... NUM_ENTRIES-1 ] = 0 };
int total_syscall_added = 0;

int line_len(char *macro_line)
{
	int len = 1;
	while(*(macro_line+len) != '\n'){
		len++;
		if(len > 1025){
			return -1;
		}
	}

	return len+1;
}

int insert_macro_line(int syscall_num, char *macro_line)
{
	int write_ret;
	struct file *f;
	char *line1raw = "#%d\n";
	char *line3 = "#end\n";
	char line1[6] = { [ 0 ... 5 ] = 0 };
    
	//opening  macro file, creating if needed. (root permission, system readable)
	f = filp_open(CUSTOM_SYSCALL_MACROS, O_CREAT|O_APPEND|O_RDWR, 0666);
	if(IS_ERR(f)){
		printk(KERN_ERR "%s: Cannot open/create macro file\n", MODNAME);
		return -1;
	}

	if(snprintf(line1, 6, line1raw, syscall_num) <= 0){
		printk(KERN_ERR "%s: snprintf line1\n", MODNAME);
		filp_close(f, NULL);
		return -1;
	} 
	
	//appending 3 macro line, as described in docs
	write_ret = kernel_write(f, line1, line_len(line1), &f->f_pos);
	if(write_ret <= 0){
		printk(KERN_ERR "%s: Cannot write first line of macro: %s, %d\n", MODNAME, line1, line_len(line1));
		filp_close(f, NULL);
		return -1;
	}
	write_ret = kernel_write(f, macro_line, line_len(macro_line), &f->f_pos);
	if(write_ret <= 0){
		printk(KERN_ERR "%s: Cannot write second line of macro\n", MODNAME);
		filp_close(f, NULL);
		return -1;
	}
	write_ret = kernel_write(f, line3, line_len(line3), &f->f_pos);
	if(write_ret <= 0){
		printk(KERN_ERR "%s: Cannot write third line of macro\n", MODNAME);
		filp_close(f, NULL);
		return -1;
	}

	filp_close(f, NULL);
	return 0;
}


int remove_macro_line(int syscall_num)
{

	return 0;
	//TODO

	struct file *f, *f_new;
	int MAX_MACRO_LEN = 2048;
	char *temp_line;
	int read_ret, write_ret;
	int charinline_read = 0;
	char *line_to_match_raw = "#%d\n";
	char line_to_match[5] = { [ 0 ... 4 ] = 0 };
	int line_to_skip_count = 3;

	f = filp_open(CUSTOM_SYSCALL_MACROS, O_RDWR, 0644);
	if(IS_ERR(f)){
		return -1;
	}
	f_new = filp_open(TEMP_MACROS_FILE, O_TRUNC|O_CREAT|O_RDWR, 0644);
	if(IS_ERR(f_new)){
		return -1;
	}

	if(snprintf(line_to_match, 5, line_to_match_raw, syscall_num) <= 0){
		return -1;
	}

	if((temp_line = kmalloc(MAX_MACRO_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc temp_line\n", MODNAME);

		return -1;
	}

	//finding the line sarting with '#num'
	//reading byte by byte all the macro file
	while(1){

		read_ret = vfs_read(f, (temp_line+charinline_read), 1, (unsigned long long *)(f->f_pos));
		if(read_ret <= 0){
			break;
		}
		charinline_read++;

		if(temp_line[charinline_read] == '\n'){

			if(line_to_skip_count == 0){
				//copy the line and conutinue
				write_ret = kernel_write(f_new, temp_line, charinline_read, (unsigned long long *)(f_new->f_pos));
				if(write_ret <= 0){
					return -1;
				}

				charinline_read = 0;
				continue;
			}

			if(line_to_skip_count < 3){
				charinline_read = 0;
				continue;
			}

			//if syscall to remove is found
			if(strncmp(line_to_match, temp_line, 5) == 0){
				//do not copy this line in the new file
				//and the next 2 lines
				line_to_skip_count--;

			}else{
				//copy the line and conutinue
				write_ret = kernel_write(f_new, temp_line, charinline_read, (unsigned long long *)(f_new->f_pos));
				if(write_ret <= 0){
					return -1;
				}

				charinline_read = 0;
				continue;
			}	
		}

	}

	filp_close(f, NULL);
	filp_close(f_new, NULL);
	kfree(temp_line);

	//remove old macro file
	/*
	if(sys_unlink(CUSTOM_SYSCALL_MACROS) == -1){
		printk(KERN_ERR "%s: Unable to unlink old macro file\n", MODNAME);
		return -1;
	}

	//rename new macro file
	if(sys_link(TEMP_MACROS_FILE, CUSTOM_SYSCALL_MACROS) == -1){
		printk(KERN_ERR "%s: Unable to rename new macro file\n", MODNAME);
		return -1;
	}
	*/

	return 0;
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

	//TODO check gor ononimous

	char *macro_line_0 = "#DEFINE %s() syscall(%d)\n";
	char *macro_line_1 = "#DEFINE %s(arg1) syscall(%d, arg1)\n";
	char *macro_line_2 = "#DEFINE %s(arg1, arg2) syscall(%d, arg1, arg2)\n";
	char *macro_line_3 = "#DEFINE %s(arg1, arg2, arg3) syscall(%d, arg1, arg2, arg3)\n";
	char *macro_line_4 = "#DEFINE %s(arg1, arg2, arg3, arg4) syscall(%d, arg1, arg2, arg3, arg4)\n";
	char *macro_line_5 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5) syscall(%d, arg1, arg2, arg3, arg4, arg5)\n";
	char *macro_line_6 = "#DEFINE %s(arg1, arg2, arg3, arg4, arg5, arg6) syscall(%d, arg1, arg2, arg3, arg4, arg5, arg6)\n";
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
			ret = snprintf(macro_line_used, DIM, macro_line_0, syscall_name, cst_entry);
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

	mutex_lock_interruptible(&mod_mutex);

	//adding entry in system call table
	if((customsys_free_indx = update_syscalltable_entry(new_syscall)) == -1){
		printk(KERN_ERR "%s: Free entry not found while adding %s\n", MODNAME, syscall_name);
		
		mutex_unlock(&mod_mutex);
		kfree(macro_line_used);
		kfree(syscall_name);
		module_put(THIS_MODULE);
		return -1;
	}

	//inserting the line in the macro file
	if(insert_macro_line(customsys_free_indx, macro_line_used) == -1){
		printk(KERN_ERR "%s: Unable to insert macro line\n", MODNAME);

		mutex_unlock(&mod_mutex);
		module_put(THIS_MODULE);
		kfree(syscall_name);
		kfree(macro_line_used);
		return -1;
	}

	//update local arrays
	if((syscall_names[total_syscall_added] = kmalloc(SYSCALL_NAME_MAX_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc syscall_names\n", MODNAME);

		mutex_unlock(&mod_mutex);
		module_put(THIS_MODULE);
		kfree(syscall_name);
		kfree(macro_line_used);
		return -1;
	}
	if(memcpy(syscall_names[total_syscall_added], syscall_name, SYSCALL_NAME_MAX_LEN) == NULL){
		printk(KERN_ERR "%s: Unable to memcpy\n", MODNAME);

		mutex_unlock(&mod_mutex);
		module_put(THIS_MODULE);
		kfree(syscall_name);
		kfree(macro_line_used);
		return -1;
	}
	syscall_cts_numbers[total_syscall_added] = customsys_free_indx;

	mutex_unlock(&mod_mutex);
	total_syscall_added++;
	kfree(macro_line_used);
	kfree(syscall_name);
	module_put(THIS_MODULE);
	return 0;
}

asmlinkage int syscall_remover(int syscall_entrynumber)
{
	int  last_syscall = syscall_cts_numbers[total_syscall_added];
	char last_syscall_name[1024];

	if(try_module_get(THIS_MODULE) == 0){
		printk(KERN_ERR "%s: Module in use, try later\n", MODNAME);
		return -1;
	}

	mutex_lock_interruptible(&mod_mutex);

	if(remove_macro_line(syscall_entrynumber) == -1){
		printk(KERN_ERR "%s: remove_macro_line returned -1\n", MODNAME);
		mutex_unlock(&mod_mutex);
		module_put(THIS_MODULE);
		return -1;
	}

	for(int i = 0; i < total_syscall_added; ++i){
		if(syscall_cts_numbers[i] == syscall_entrynumber){
			syscall_cts_numbers[i] = syscall_cts_numbers[total_syscall_added-1];
			syscall_cts_numbers[total_syscall_added-1] = 0;

			kfree(syscall_names[i]);
			syscall_names[i] = syscall_names[total_syscall_added-1];
			syscall_names[total_syscall_added-1] = 0;
			total_syscall_added--;
			break;
		}
	}

	
	mutex_unlock(&mod_mutex);

	module_put(THIS_MODULE);

	printk("TODO");

	return 0;
}

static int __init install(void)
{

	//TODO create the header file, add the adder entry.

	//when a new syscall has to be added, the header created has to be included,
	//then call syscall(NR, void* syscall_toadd_definedinmodule), and the new
	//entry will be inserted.
	int add_indx, rem_indx;
	char *adder_macro_line_raw = "#DEFINE syscall_remover(arg1) syscall(%d, arg1)\n";
	char *remover_macro_line_raw = "#DEFINE syscall_adder(arg1, arg2, arg3, arg4, arg5) syscall(%d, arg1, arg2, arg3, arg4, arg5)\n";
	char adder_macro_line[300] = { [ 0 ... 299 ] = 0 };
	char remover_macro_line[512] = { [ 0 ... 511 ] = 0 };

	//setting directory dir
	if(snprintf(CUSTOM_SYSCALL_MACROS, 512, CUSTOM_SYSCALL_MACROS_RAW, MACRO_DIR) <= 0){
		return -1;
	}
	if(snprintf(TEMP_MACROS_FILE, 512, TEMP_MACROS_FILE_RAW, MACRO_DIR) <= 0){
		return -1;
	}

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
	if((add_indx = update_syscalltable_entry(syscall_adder)) == -1){
		printk(KERN_DEBUG "%s: Syscall_adder not inserted\n", MODNAME);
		return -1;
	}

	//adding sys remover
	if((rem_indx = update_syscalltable_entry(syscall_remover)) == -1){
		printk(KERN_DEBUG "%s: Syscall_remover not inserted\n", MODNAME);
		return -1;
	}

	//formatting macro lines for adder and remover
	if(snprintf(adder_macro_line, 1024, adder_macro_line_raw, add_indx) <= 0){
		return -1;
	}
	if(snprintf(remover_macro_line, 1024, remover_macro_line_raw, rem_indx) <= 0){
		return -1;
	}

	//insert_macro_line for adder & remover
	if(insert_macro_line(add_indx, adder_macro_line) == -1){
		printk(KERN_ERR "%s: Unable to insert macro line for adder\n", MODNAME);
		return -1;
	}
	if(insert_macro_line(rem_indx, remover_macro_line) == -1){
		printk(KERN_ERR "%s: Unable to insert macro line for remover\n", MODNAME);
		return -1;
	}

	printk(KERN_DEBUG "%s: syscall_adder & syscall_remover added correctly\n", MODNAME);

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