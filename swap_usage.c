/* swap-usage.c - Check swap usage for each process
 *
 * Copyright (C) 2001, 2002 Mission Critical Linux, Inc.
 * Copyright (C) 2002, 2003, 2004, 2005 David Anderson
 * Copyright (C) 2002, 2003, 2004, 2005 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "defs.h"

int _init(void);
int _fini(void);

void cmd_swap_usage(void);

static unsigned int swap_usage_offset;

static struct command_table_entry command_table[] = {
	{ "swap_usage", cmd_swap_usage, NULL, 0 },
	{ NULL }
};

int 
_init(void)
{ 
        register_extension(command_table);
	return 1;
}
 
int 
_fini(void) 
{ 
	return 1;
}

void
show_swap_usage(struct task_context *tc) 
{
	struct task_mem_usage task_mem_usage, *tm;
	tm = &task_mem_usage;
	get_task_mem_usage(tc->task, tm);
	ulong swap_usage;

	readmem((tm->mm_struct_addr + swap_usage_offset), KVADDR, &swap_usage, sizeof(void *), "mm_counter_t", QUIET|RETURN_ON_ERROR);
        fprintf(fp, "%5ld  %5ld  %5s\n",
		tc->pid, swap_usage  << (PAGESHIFT()-10), tc->comm);
}


void
cmd_swap_usage(void)
{
	struct task_context *tc;
	int i;

	if (MEMBER_EXISTS("mm_struct", "_swap_usage")) {
		swap_usage_offset = MEMBER_OFFSET("mm_struct", "_swap_usage");

		fprintf(fp,
		    "PID     SWAP     COMM\n");

		tc = FIRST_CONTEXT();
		for (i = 0; i < RUNNING_TASKS(); i++, tc++)
			show_swap_usage(tc);

		fprintf(fp, "\n");

	} else {
		fprintf(fp, "This extension only works under RHEL 6\n");
	}
}
