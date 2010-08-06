#ifndef ROS_KERN_SYSCALL_H
#define ROS_KERN_SYSCALL_H
#ifndef ROS_KERNEL
# error "This is ROS kernel header; user programs should not #include it"
#endif

#include <ros/syscall.h>
#include <process.h>

#define SYSTRACE_ON					0x01
#define SYSTRACE_LOUD				0x02
#define SYSTRACE_ALLPROC			0x04

#define MAX_NUM_TRACED				10
#define MAX_SYSTRACES				1024

/* Consider cache aligning this */
struct systrace_record {
	uint64_t		timestamp;
	uintreg_t		syscallno;
	int				pid;
	uint32_t		coreid;
	uint32_t		vcoreid;
};

intreg_t syscall(struct proc *p, uintreg_t num, uintreg_t a1, uintreg_t a2,
                 uintreg_t a3, uintreg_t a4, uintreg_t a5);
intreg_t syscall_async(env_t* e, syscall_req_t *syscall);
intreg_t process_generic_syscalls(env_t* e, size_t max);

/* Tracing functions */
void systrace_start(bool silent);
void systrace_stop(void);
int systrace_reg(bool all, struct proc *p);
int systrace_dereg(bool all, struct proc *p);
void systrace_print(bool all, struct proc *p);
void systrace_clear_buffer(void);

#endif /* !ROS_KERN_SYSCALL_H */
