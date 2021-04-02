

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
	char *line1raw = "//%d\n";
	char *line3 = "//end\n";
	char line1[7] = { [ 0 ... 6 ] = 0 };
    
	//opening  macro file, creating if needed. (root permission, system readable)
	f = filp_open(CUSTOM_SYSCALL_MACROS, O_CREAT|O_APPEND|O_RDWR, 0666);
	if(IS_ERR(f)){
		printk(KERN_ERR "%s: Cannot open/create macro file\n", MODNAME);
		return -1;
	}

	if(snprintf(line1, 7, line1raw, syscall_num) <= 0){
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


//TODO, maybe later on...
int remove_macro_line(int syscall_num)
{

	struct file *f;
	int MAX_MACRO_LEN = 2048;
	char *temp_line;
	int read_ret;
	int charinline_read = 0;
	char *line_to_match_raw = "//%d\n";
	char line_to_match[5] = { [ 0 ... 4 ] = 0 };
	int line_to_skip_count = 3;

	f = filp_open(CUSTOM_SYSCALL_MACROS, O_RDWR, 0644);
	if(IS_ERR(f)){
		return -1;
	}

	if(snprintf(line_to_match, 5, line_to_match_raw, syscall_num) <= 0){
		return -1;
	}

	if((temp_line = kmalloc(MAX_MACRO_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc temp_line\n", MODNAME_R);

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

			printk(KERN_DEBUG "%s: temp_line read: %s\n", MODNAME_R, temp_line);

			//TODO buffer all the lines and the recopy them in the macrofile ?
			if(line_to_skip_count == 0){
				//buffer the line
				charinline_read = 0;
				continue;
			}

			if(line_to_skip_count < 3){
				//skip the line
				charinline_read = 0;
				continue;
			}

			//if syscall to remove is found
			if(strncmp(line_to_match, temp_line, 5) == 0){
				//do not buffer the line
				line_to_skip_count--;

			}else{
				//buffer the line
				charinline_read = 0;
				continue;
			}	
		}

	}

	filp_close(f, NULL);
	kfree(temp_line);
	//TODO, reopen the file in TRUNC mode and write the buffered lines ?

	return 0;
}


int find_syscalltable_free_entry(void)
{
	void **temp_addr = (void **)sys_call_table_address;
	int i;

	for(i = 0; i < NUM_ENTRIES; i += 1){

		if(temp_addr[i] == (void *)sysnisyscall_addr){
			break;
		}
	}
	if(i == NUM_ENTRIES){
		return -1;
	}
	return i;
}

int update_syscalltable_entry(void* custom_syscall, char* syscall_name)
{
	int syst_entry;
	unsigned long cr0;
	int SYSCALL_NAME_MAX_LEN = 1024;

	syst_entry = find_syscalltable_free_entry();
	if(syst_entry == -1){
		printk(KERN_DEBUG "%s: Free entry not found\n", MODNAME);
		return -1;
	}
	printk(KERN_DEBUG "%s: Index found: %d\n",MODNAME, syst_entry);

	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);
	((void **)sys_call_table_address)[syst_entry] = custom_syscall;
	write_cr0(cr0);

	//update local arrays
	if((syscall_names[total_syscall_added] = kmalloc(SYSCALL_NAME_MAX_LEN, GFP_KERNEL)) == NULL){
		printk(KERN_ERR "%s: Unable to kmalloc syscall_names\n", MODNAME);
		return -1;
	}
	if(memcpy(syscall_names[total_syscall_added], syscall_name, SYSCALL_NAME_MAX_LEN) == NULL){
		printk(KERN_ERR "%s: Unable to memcpy\n", MODNAME);
		return -1;
	}
	syscall_cts_numbers[total_syscall_added] = syst_entry;
	total_syscall_added++;

	return syst_entry;
}