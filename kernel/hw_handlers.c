/*
 *
 * Harware Handler Interface  
 *
 */
#include "hw_handlers.h"
#include "mmap.h"
#include "memory.h"
#include "interrupt.h"
#include "klibc.h"
#include "vm.h"
#include "file.h"
#include "process.h"
#include "scheduler.h"

#define ENABLE_MAX_INTERRUPT
#define ENABLE_MAX_INTERRUPT_COUNT 5
int interrupt_count = 0;

/* copy vector table from wherever QEMU loads the kernel to 0x00 */
void init_vector_table(void) {

	/* Primary Vector Table */
	mmio_write(0x00, BRANCH_INSTRUCTION);
	mmio_write(0x04, BRANCH_INSTRUCTION);
	mmio_write(0x08, BRANCH_INSTRUCTION);
	mmio_write(0x0C, BRANCH_INSTRUCTION);
	mmio_write(0x10, BRANCH_INSTRUCTION);
	mmio_write(0x14, BRANCH_INSTRUCTION);
	mmio_write(0x18, BRANCH_INSTRUCTION);
	mmio_write(0x1C, BRANCH_INSTRUCTION);

	/* Secondary Vector Table */
	mmio_write(0x20, &reset_handler);
	mmio_write(0x24, &undef_instruction_handler);
	mmio_write(0x28, &software_interrupt_handler);
	mmio_write(0x2C, &prefetch_abort_handler);
	mmio_write(0x30, &data_abort_handler);
	mmio_write(0x34, &reserved_handler);
	mmio_write(0x38, &irq_handler);
	mmio_write(0x3C, &fiq_handler);
}

/* handlers */
void reset_handler(void) {
	os_printf("RESET HANDLER\n");
	_Reset();
}

void __attribute__((interrupt("UNDEF"))) undef_instruction_handler(void) {
#ifdef ENABLE_MAX_INTERRUPT
	if (++interrupt_count > ENABLE_MAX_INTERRUPT_COUNT)
		return;
#endif
	os_printf("UNDEFINED INSTRUCTION HANDLER\n");
}

/*
 * enters sp mode and transfers the sp and lr registers to r0 and
 * r1 respectively
 */
void hw_get_spmode_sp_lr();

long __attribute__((interrupt("SWI"))) software_interrupt_handler(void) {
	jmp_buf pre_swi_state;

	// look at assembly - move r3 first
	asm volatile("MOV %0, r3":"=r"(pre_swi_state.R3)::);
	asm volatile("MOV %0, r0":"=r"(pre_swi_state.R0)::);
	asm volatile("MOV %0, r1":"=r"(pre_swi_state.R1)::);
	asm volatile("MOV %0, r2":"=r"(pre_swi_state.R2)::);
	asm volatile("MOV %0, r4":"=r"(pre_swi_state.R4)::);
	asm volatile("MOV %0, r5":"=r"(pre_swi_state.R5)::);
	asm volatile("MOV %0, r6":"=r"(pre_swi_state.R6)::);
	asm volatile("MOV %0, r7":"=r"(pre_swi_state.R7)::);
	asm volatile("MOV %0, r8":"=r"(pre_swi_state.R8)::);
	asm volatile("MOV %0, r9":"=r"(pre_swi_state.R9)::);
	asm volatile("MOV %0, r10":"=r"(pre_swi_state.R10)::);
	asm volatile("MOV %0, r11":"=r"(pre_swi_state.R11)::);
	asm volatile("MOV %0, r12":"=r"(pre_swi_state.R12)::);

	hw_get_spmode_sp_lr();

	asm volatile("MOV %0, r0":"=r"(pre_swi_state.R13)::);
	asm volatile("MOV %0, r1":"=r"(pre_swi_state.R14)::);

	long ret = STATUS_FAIL;
	uint32_t r0 = pre_swi_state.R0;
	uint32_t r1 = pre_swi_state.R1;
	uint32_t r2 = pre_swi_state.R2;
	uint32_t r3 = pre_swi_state.R3;
	uint32_t call_num = pre_swi_state.R7;
	struct vas *prev_vas = vm_get_current_vas();

	vm_use_kernel_vas();

	os_printf("SWI HANDLER [0x%X(%d)]\n", call_num, call_num);

	switch (call_num) {
		case SYSCALL_DELETE: {
			os_printf("SYSCALL_DELETE\n");
			ret = kdelete((char*) r0);
			break;
		}
		case SYSCALL_OPEN: {
			os_printf("SYSCALL_OPEN\n");
			ret = kopen((char*) r0, (char) r1);
			break;
		}
		case SYSCALL_CREATE: {
			os_printf("SYSCALL_CREATE\n");
			ret = kcreate((char*) r0, (char) r1, (int) 0);
			break;
		}
		case SYSCALL_MKDIR: {
			os_printf("SYSCALL_MKDIR\n");
			ret = kcreate((char*) r0, 'w', 1);
			break;
		}
		case SYSCALL_READ: {
			os_printf("SYSCALL_READ\n");
			ret = kread((int) r0, (void*) r1, (int) r2);
			break;
		}
		case SYSCALL_WRITE: {
			os_printf("SYSCALL_WRITE\n");
			ret = kwrite(r0, r1, r2);
			break;
		}
		case SYSCALL_CLOSE: {
			os_printf("SYSCALL_CLOSE\n");
			ret = kclose(r0);
			break;
		}
		case SYSCALL_SEEK: {
			os_printf("SYSCALL_SEEK\n");
			ret = kseek(r0, r1);
			break;
		}
		case SYSCALL_COPY: {
			os_printf("SYSCALL_COPY\n");
			ret = kcopy(r0, r1, r2);
			break;
		}
		case SYSCALL_LS: {
			os_printf("SYSCALL_LS\n");
			ret = kls(r0);
			break;
		}
		case SYSCALL_SET_PERM: {
			LOG("SYSCALL_SET_PERM\n");
			vm_use_kernel_vas();
			os_printf("Set permission system call called!\n");
			os_printf("Yet to be implemented\n");
			ret = STATUS_FAIL;
			break;
		}
		case SYSCALL_MEM_MAP: {
			LOG("SYSCALL_MEM_MAP\n");
			os_printf("Memory map system call called!\n");
			os_printf("Yet to be implemented\n");
			ret = STATUS_FAIL;
			break;
		}
		case SYSCALL_PRCS_LISTEN: {
			LOG("SYSCALL_PRCS_LISTEN\n");
			ret = sched_register_callback_handler(
					(sched_msg_callback_handler) r0);
			break;
		}
		case SYSCALL_PRCS_EMIT: {
			LOG("SYSCALL_PRCS_EMIT\n");
			ret = sched_post_message(r0, r1, r2, r3);
			break;
		}
		case SYSCALL_PRCS_YIELD: {
			LOG("SYSCALL_PRCS_YIELD\n");
			LOG("Yield - return to 0x%X\n", pre_swi_state.R14);
			sched_update_task_state(sched_get_active_tid(), &pre_swi_state);
			ret = sched_yield();
			break;
		}
		case SYSCALL_MALLOC: {
			os_printf("SYSCALL_MALLOC\n");
			ret = umalloc(r0);
			break;
		}
		case SYSCALL_CALLOC: {
			os_printf("SYSCALL_CALLOC\n");
			ret = ucalloc(r0, r1);
			break;
		}
		case SYSCALL_FREE: {
			os_printf("SYSCALL_FREE\n");
			ufree(r0);
			ret = STATUS_OK;
			break;
		}
		case SYSCALL_SWITCH: {
			os_printf("SYSCALL_SWITCH\n");
			sched_remove_task(sched_get_active_tid());
			ret = sched_yield();
			break;
		}
		case SYSCALL_PRCS_EXIT: {
			os_printf("SYSCALL_PRCS_EXIT\n");
			sched_remove_task(sched_get_active_tid());
			ret = sched_yield();
			break;
		}
		case SYSCALL_PRINTF: {
#define PRINTF_COPY_ADDR 0x8f000000
			vm_map_shared_memory(KERNEL_VAS, (void*) PRINTF_COPY_ADDR, prev_vas,
					(void*) r0, VM_PERM_PRIVILEGED_RW);
			os_printf("[%d] %s", (char*) r1, r0);
			vm_free_mapping(KERNEL_VAS, (void*) PRINTF_COPY_ADDR);
			ret = STATUS_OK;
			break;
		}
		default:
			os_printf("SYSCALL_UNDEFINED\n");
			break;
	}

	vm_enable_vas(prev_vas);

	return ret;
}

void __attribute__((interrupt("ABORT"))) prefetch_abort_handler(void) {
#ifdef ENABLE_MAX_INTERRUPT
	if (++interrupt_count > ENABLE_MAX_INTERRUPT_COUNT)
		return;
#endif
	os_printf("PREFETCH ABORT HANDLER\n");
}

void __attribute__((interrupt("ABORT"))) data_abort_handler(void) {
#ifdef ENABLE_MAX_INTERRUPT
	if (++interrupt_count > ENABLE_MAX_INTERRUPT_COUNT)
		return;
#endif
	os_printf("DATA ABORT HANDLER\n");
	int pc, lr, sp, fp;
	// not sure this is correct syntax, did we [Spring 2015 do this?]
	asm volatile("mov %0, pc" : "=r" (pc));
	asm volatile("mov %0, lr" : "=r" (lr));
	asm volatile("mov %0, sp" : "=r" (sp));
	asm volatile("mov %0, fp" : "=r" (fp));
	os_printf("HANDLER: pc=%x, lr=%x, sp=%x, fp=%x\n", pc, lr, sp, fp);

	// Get the DSFR
	int dsfr;
	asm volatile("MRC p15, 0, %0, c5, c0, 0" : "=r" (dsfr));
	os_printf("DSFR: 0x%X\n", dsfr);

	switch (dsfr) {
		case 6: // Access bit.
			// Set it to 1 so we don't get notified again.
			// TODO: The eviction policy will listen to this.
			*((unsigned int*) (V_L1PTBASE + 2 * PAGE_TABLE_SIZE)) |= (1 << 4);
			break;
		default:
			break;
	};
}

void reserved_handler(void) {
	os_printf("RESERVED HANDLER\n");
}

// the attribute automatically saves and restores state
void __attribute__((interrupt("IRQ"))) irq_handler(void) {

	os_printf("IRQ HANDLER\n");
	disable_interrupts()
	;

	// Discover source of interrupt
	int i = 0;
	// do a straight run through the VIC_INT_STATUS to determine
	// which interrupt lines need to be tended to
	for (i = 0; i < MAX_NUM_INTERRUPTS; i++) {
		// is the line active?
		if ((1 << i) & mmio_read(VIC_IRQ_STATUS)) {
			// activate that specific handler
			handle_irq_interrupt(i);
		}
	}
	// we've gone through the VIC and handled all active interrupts
	enable_interrupts();
}

void __attribute__((interrupt("FIQ"))) fiq_handler(void) {
#ifdef ENABLE_MAX_INTERRUPT
	if (++interrupt_count > ENABLE_MAX_INTERRUPT_COUNT)
		return;
#endif
	os_printf("FIQ HANDLER\n");
// FIQ handler returns from the interrupt by executing:
// SUBS PC, R14_fiq, #4
}
