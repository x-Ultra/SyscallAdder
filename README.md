# SyscallAdder

Linux kernel module that can be used to add custom system call in a more user-friendly way.

This module will add 2 system call:

1. **syscall_adder((void \*)custom\_syscall\_addr, char \*syscall\_name, int num\_parameters)**: will check if there is a free entry on the syscall table and if so, the syscall will be inserted. There will be inserted a _MACRO_ in a file (located at \~/custom_syscall_macros.h). This macro, **when imported in the user c file where the cusom syscall is used**, will make possible calling the new syscall like: `custom_syscall(...)`.

2. **syscall_remover(int custom_syscall_name)**: This system call will simply delete a custom system call inserted previously.

## Usage

Fist of all you have to download and install the syscall_adder module:
1. `git clone https://github.com/x-Ultra/SyscallAdder`
2. `cd SyscallAdder`
3. Edit `#define MACRO_DIR "/dir/to/macro/file"` at line 35 of syscalladder.c
4. `sudo ./install`
5. To uninstall `sudo ./uninstall </dir/to/macro/file>`

Then, to add a new system call:

1. Open the 'your_syscall.c' file in 'template' folder
2. Edit the template as you need
3. While adding a new syscall remember to use an integer variable to maintain the index of the added syscall (line 15), to use in the removing procedure.
4. Rename the your_syscall.c as you want
5. To install your module (and your system calls): `sudo ./insert_syscall`
6. To remove: `sudo ./remove_syscall`

To use you new systemcall just import the macro file `/dir/to/macro/file/custom_syscall_macros.h` into your user C file.

## The MACRO file

Let's suppose to add the system call 'my_sys(int arg1)', and that the syscall\_adder will insert at in the (system call table) index 187.
The macro file will look like that:

```C
//187
#DEFINE my_sys(arg1) syscall(187, arg1)
//end
```

This means that the syscall\_adder has to know the number of parameters in order to create the appropiate macro. That's it.