#ifndef ROS_ARCH_TRAP_H
#define ROS_ARCH_TRAP_H

#ifdef __riscv64
# define SIZEOF_HW_TRAPFRAME (36*8)
#else
# define SIZEOF_HW_TRAPFRAME (36*4)
#endif

#ifndef __ASSEMBLER__

#ifndef ROS_KERN_TRAP_H
#error "Do not include include arch/trap.h directly"
#endif

#include <ros/trapframe.h>
#include <arch/arch.h>

/* Kernel message interrupt vector.  ignored, for the most part */
#define I_KERNEL_MSG 255
#warning "make sure this poke vector is okay"
/* this is for an ipi that just wakes a core, but has no handler (for now) */
#define I_POKE_CORE 254

static inline bool in_kernel(struct hw_trapframe *hw_tf)
{
	return hw_tf->sr & SR_PS;
}

static inline uintptr_t get_hwtf_pc(struct hw_trapframe *hw_tf)
{
	#warning "fix me"
	return 0;
	//return hw_tf->tf_rip;
}

static inline uintptr_t get_hwtf_fp(struct hw_trapframe *hw_tf)
{
	/* do you even have frame pointers?  this is used for backtrace, but if you
	 * don't use FPs, we'll need to change up our parameters or something. */
	#warning "fix me"
	return 0;
	//return hw_tf->tf_rbp;
}

static inline uintptr_t get_swtf_pc(struct sw_trapframe *sw_tf)
{
	#warning "fix me"
	return 0;
	//return sw_tf->tf_rip;
}

static inline uintptr_t get_swtf_fp(struct sw_trapframe *sw_tf)
{
	#warning "fix me"
	return 0;
	//return sw_tf->tf_rbp;
}

static inline void __attribute__((always_inline))
set_stack_pointer(uintptr_t sp)
{
	asm volatile("move sp, %0" : : "r"(sp) : "memory");
}

static inline void __attribute__((always_inline))
set_frame_pointer(uintptr_t fp)
{
	#warning "brho is just guessing here."
	asm volatile("move fp, %0" : : "r"(fp) : "memory");
}

void handle_trap(struct hw_trapframe *hw_tf);
int emulate_fpu(struct hw_trapframe *hw_tf);

#endif

#endif
