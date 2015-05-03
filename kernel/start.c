/*
 *  A bit of background:
 *  - The ARM architecture has 7 modes of operation:
 *      + USR - user mode
 *      + FIQ - processing "fast" interrupts
 *      + IRQ - processing "normal" interrupts
 *      + SVC - proctected mode for OS
 *      + UND - processing an undefined instruction exception
 *      + SYS - also protecteed mode for OS --if anyone wants to clarify, feel free--
 *
 *  - These modes can be entered or exited by modifying the CPSR (status register), first 5 bits
 *	+ 0b10000 = user mode
 *	+ 0b10001 = FIQ (fast interrupt) mode
 *	+ 0b10010 = IRQ (normal interrupt) mode
 *	+ 0b10011 = SVC (supervisor, or, OS) mode
 *	(others...)
 */

#include <stdint.h>
#include "hw_handlers.h"
#include "global_defs.h"
#include "argparse.h"
#include "interrupt.h"
#include "mmap.h"
#include "process.h"
#include "memory.h"
#include "drivers/uart.h"
#include "drivers/mmci.c"
#include "klibc.h"
#include "vm.h"
#include "include/open_table.h" //to initialize fs opentable
#include "mem_alloc.h"
#include "tests.h"
#include "drivers/timer.h"
#include "scheduler.h"

// Tests
#include "tests/test_priority_queue.h"
#include "tests/test_hash_map.h"
#include "tests/test_mem_alloc.h"
#include "tests/test_vm.h"
#include "tests/test_kthreads.h"
#include "tests/test_process.h"
#include "tests/test_sched_process.h"

#define UART0_IMSC (*((volatile uint32_t *)(UART0_ADDRESS + 0x038)))

extern int process_global_init();

void uart_handler(void *null) {
	print_uart0("uart0!\n");
}

// This start is what u-boot calls. It's just a wrapper around setting up the
// virtual memory for the kernel.
void start(uint32_t *p_bootargs) {
	// Initialize the virtual memory
	print_uart0("Enabling MMU...\n");
	INFO("%X\n", *p_bootargs);
	vm_init();
	INFO("Initialized VM datastructures.\n");
	mmap(p_bootargs);
}

// This start is what starts the kernel. Note that virtual memory is enabled
// at this point (And running, also, in the kernel's VAS).
void start2(uint32_t *p_bootargs) {

	// Initialize
	// ----------
	// Setup all of the exception handlers... (hrm, interaction with VM?)
	init_vector_table();
	init_heap();
	sched_init();
	kfs_init(0, 0, 0);
	process_global_init();

	//Test: UART0 mapped to the correct virtual address
	print_uart0("MMU enabled\n");
	print_uart0("\nCourseOS!\n");

	INFO("Bootargs: %X\n", *p_bootargs);

	// Test cases
	// ----------
//	run_vm_tests();
//	run_mem_alloc_tests();
//	run_prq_tests();
//	run_hmap_tests();
//	run_kthr_tests();
//	run_sched_prcs_tests();
    run_prcs_tests();
//	run_fs_tests();

	/*
	 4-15-15: 	#Prakash: 	What happens if we let the program load here?
	 Let's make argparse_process() do its thing

	 Note: As of 4-15-15 this fails horribly with hello.o not being
	 recognized as an ELF file and DATA ABORT HANDLER being syscalled
	 */

//	argparse_process(p_bootargs);
	print_uart0("done parsing atag list\n");

	while (1) {
		// wait
	}
}
