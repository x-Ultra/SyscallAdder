#define MODNAME "syscall_adder"
#define MODNAME_R "syscall_remover"
#define NUM_ENTRIES 1024
//This file will contain the macros that will be used to call
//a custom syscall as if it was an 'embedded' one.
//Eg. sys_call_adder("syscall_name") will be called inside the kernel module
//that contains the new syscall to add. (TODO automate the process)
#define MACRO_DIR "/home/ezio"
#define CUSTOM_SYSCALL_MACROS_RAW "%s/custom_syscall_macros.h"

MODULE_AUTHOR("Ezio Emanuele Ditella");
MODULE_DESCRIPTION("This module will create a syscall that will be used \
					to add new syscalls.");


unsigned long sys_call_table_address = 0;
unsigned long sysnisyscall_addr = 0;
DEFINE_MUTEX(mod_mutex);

char CUSTOM_SYSCALL_MACROS[512];
char *syscall_names[NUM_ENTRIES] = { [ 0 ... NUM_ENTRIES-1 ] = 0 };
int syscall_cts_numbers[NUM_ENTRIES] = { [ 0 ... NUM_ENTRIES-1 ] = 0 };
int total_syscall_added = 0;
int add_indx, rem_indx;
int uninstalling = 0;