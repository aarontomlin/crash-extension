/* swap-usage.c - Check actual swap consumption for each process
 *
 * Aaron Tomlin <atomlin@redhat.com>
 *
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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

#define MEMBER_FOUND 1
#define MEMBER_NOT_FOUND 0
#define PRINT_HEADER() \
		fprintf(fp, \
	"PID     SWAP     COMM\n");

int _init(void);
int _fini(void);

void cmd_swap_usage(void);
char *help_swap_usage[];

static unsigned int swap_usage_offset;

static struct command_table_entry command_table[] = {
	{ "swap_usage", cmd_swap_usage, help_swap_usage, 0 },
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
show_swap_usage(struct task_context *tc, unsigned int exists) 
{
	struct task_mem_usage task_mem_usage, *tm;
	tm = &task_mem_usage;
	get_task_mem_usage(tc->task, tm);
	physaddr_t paddr;
	ulong mm;
	ulong vma;
	ulong vm_start;
	ulong vm_end;
	ulong vm_next;
	ulong swap_usage = 0;

	readmem(tc->task + OFFSET(task_struct_mm), KVADDR,
        &mm, sizeof(void *), "mm_struct mm", FAULT_ON_ERROR);

	if (!mm)
		return;

	switch (exists) {
	case MEMBER_FOUND:

		readmem((mm + swap_usage_offset), KVADDR, 
		&swap_usage, sizeof(void *), "mm_counter_t", FAULT_ON_ERROR);

		break;

	case MEMBER_NOT_FOUND:
	default:

		readmem(mm + OFFSET(mm_struct_mmap), KVADDR,
		&vma, sizeof(void *), "mm_struct mmap", FAULT_ON_ERROR);

		for (; vma; vma = vm_next) {

			readmem(vma + OFFSET(vm_area_struct_vm_start), KVADDR, 
			&vm_start, sizeof(void *), "vm_area_struct vm_start", FAULT_ON_ERROR);

			readmem(vma + OFFSET(vm_area_struct_vm_end), KVADDR,
			&vm_end, sizeof(void *), "vm_area_struct vm_end", FAULT_ON_ERROR);

			readmem(vma + OFFSET(vm_area_struct_vm_next), KVADDR,
			&vm_next, sizeof(void *), "vm_area_struct vm_next", FAULT_ON_ERROR);

			while (vm_start < vm_end) {
				if (!uvtop(tc, vm_start, &paddr, 0)) {

					if (paddr && !(paddr & _PAGE_FILE)) {
						swap_usage++;
					}
				}
				vm_start += PAGESIZE();
			}
		}
	}
	fprintf(fp, "%5ld  %5ld\t%s\n",
	tc->pid, swap_usage  << (PAGESHIFT()-10), tc->comm);
}


void
cmd_swap_usage(void)
{
	struct task_context *tc;
	int i;
	int c;
	ulong value;
	int subsequent = 0;
	unsigned int exists = MEMBER_NOT_FOUND;

	if (MEMBER_EXISTS("mm_struct", "_swap_usage")) {
		swap_usage_offset = MEMBER_OFFSET("mm_struct", "_swap_usage");
		exists = MEMBER_FOUND;
	}

	while ((c = getopt(argcnt, args, "")) != EOF) {
		switch (c) {
		default:
			argerrs++;
			break;
		}
	}

	if (argerrs)
		cmd_usage(pc->curcmd, SYNOPSIS);

        if (!args[optind]) {
                PRINT_HEADER();
                tc = FIRST_CONTEXT();
                for (i = 0; i < RUNNING_TASKS(); i++, tc++) {
                        if (!is_kernel_thread(tc->task))
                                show_swap_usage(tc, exists);
                }   
		return;
        }

	PRINT_HEADER();
	while (args[optind]) {
		switch (str_to_context(args[optind], &value, &tc)) {
		case STR_PID:
			for (tc = pid_to_context(value); tc; tc = tc->tc_next) {
				if (!is_kernel_thread(tc->task)) {
					show_swap_usage(tc, exists);
				} else {
					error(INFO, "only specify a user task or pid: %s\n",
						args[optind]);
				}
			}
			break;

		case STR_TASK:
			for (; tc; tc = tc->tc_next) {
				if (!is_kernel_thread(tc->task)) {
					show_swap_usage(tc, exists);
				} else {
					error(INFO, "only specify a user task or pid: %s\n",
						args[optind]);
				}
			}
			break;

		case STR_INVALID:
			error(INFO, "invalid task or pid value: %s\n",
				args[optind]);
			break;
		}

		subsequent++;
		optind++;
	}
}

char *help_swap_usage[] = {
	"swap_usage",
	"Returns the actual swap consumption of a user process",
	"[pid | taskp]",

	"  This command obtains the swap consumption (in kilobytes) of a user process.",
	"  Supported on x86_64 only.",
	"\nEXAMPLE",
	"    Show the swap consumption for pid 1288, 1232 and 663:\n",
	"  	crash> swap_usage 1288 1232 663",
	"	PID     SWAP     COMM",
	"	 1288   2100	audispd",
	"	 1232    188	bluetoothd",
	"	  663   3384	udevd",
	"	crash>", 
	" ",
	"If no arguments are specified, every user process will be checked.",
	NULL
};
