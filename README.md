# SyscallAdder

Linux kernel module that can be used to add custom system call in a more user-friendly way.

This module will add 2 system call:

1. **syscall_adder((void \*)custom\_syscall\_addr, char \*syscall\_name, int num\_parameters)**: will check if there is a free entry on the syscall table and if so, the syscall will be inserted. There will be inserted a _MACRO_ in a file (located at \~/custom_syscall_macros.h). This macro, **when imported in the user c file where the cusom syscall is used**, will make possible calling the new syscall like: `custom_syscall(...)`.

2. **syscall_remover(int custom_syscall_name)**: This system call will simply delete a custom system call inserted previously.

## Usage

Fist of all you have to download and install the syscall_adder module:
1. `git clone https://github.com/x-Ultra/SyscallAdder`
2. `cd SyscallAdder`
3. Edit `#define MACRO_DIR "dir/to/macro/file"` at line 35 of syscalladder.c
4. `sudo insmod syscalladder.ko`

Then, to add a new system call:

1. Open the new_syscall_template.c in 'syscall_template' folder
2. Fill the 'your_custom_syscall' function with your system call logic
3. Rename as you want: the preavious function name, the macro line '#DEFINE YOUR_SYSCALL_NAME'
4. Rename the new_syscall_template.c file with the name of your system call **and** the line `obj-m += new_syscall_template.o` to `obj-m += renamed_c_file.o`

To use you new systemcall just import the macro file `~/custom_syscall_macros.h` into your user C file.

## The MACRO file

Let's suppose to add the system call 'my_sys(int arg1)', and that the syscall\_adder will insert at in the (system call table) index 187.
The macro file will look like that:

```C
#187
#DEFINE my_sys(arg1) syscall(187, arg1)
#end
```

This means that the syscall\_adder has to know the number of parameters in order to create the appropiate macro. That's it.