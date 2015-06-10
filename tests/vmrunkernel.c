#include <stdio.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arch/arch.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ros/syscall.h>
#include <sys/mman.h>
#define PML4_SHIFT		39
#define PML3_SHIFT		30
#define PML2_SHIFT		21
#define PML1_SHIFT		12
#define BITS_PER_PML	9

/* PTE reach is the amount of VM an entry can map, either as a jumbo or as
 * further page tables.  I'd like to write these as shifts, but I can't please
 * both the compiler and the assembler. */
#define PML4_PTE_REACH	(0x0000008000000000)	/* No jumbos available */
#define PML3_PTE_REACH	(0x0000000040000000)	/* 1 GB jumbos available */
#define PML2_PTE_REACH	(0x0000000000200000)	/* 2 MB jumbos available */
#define PML1_PTE_REACH	(0x0000000000001000)	/* aka, PGSIZE */

/* Reach is the amount of VM a table can map, counting all of its entries.
 * Note that a PML(n)_PTE is a PML(n-1) table. */
#define PML3_REACH		(PML4_PTE_REACH)
#define PML2_REACH		(PML3_PTE_REACH)
#define PML1_REACH		(PML2_PTE_REACH)

/* PMLx(la, shift) gives the 9 bits specifying the la's entry in the PML
 * corresponding to shift.  PMLn(la) gives the 9 bits for PML4, etc. */
#define PMLx(la, shift)	(((uintptr_t)(la) >> (shift)) & 0x1ff)
#define PML4(la) 		PMLx(la, PML4_SHIFT)
#define PML3(la) 		PMLx(la, PML3_SHIFT)
#define PML2(la) 		PMLx(la, PML2_SHIFT)
#define PML1(la) 		PMLx(la, PML1_SHIFT)

static void *mmap_blob;
unsigned long long stack[1024];
volatile int tr, rr, done;
volatile int state;
int debug;

unsigned long long *p512, *p1, *p2m;

void *talk_thread(void *arg)
{
	printf("talk thread ..\n");
	int c;
	
	// This is a a bit odd but getchar() is not echoing characters.
	// That's good for us but makes no sense.
	while (!done && (c = getchar())) {
		int i;
		if (debug) printf("Set rr to 0x%x\n", c | 0x80);
		rr = c | 0x80;
		if (debug) printf("rr 0x%x tr 0x%x\n", rr, tr);
		while (! tr)
			;
		if (debug) printf("tr 0x%x\n", tr);
		putchar(tr & 0x7f);
		tr = 0;
	}
	rr = 0;	
	return NULL;
}

pthread_t *my_threads;
void **my_retvals;
int nr_threads = 2;

int main(int argc, char **argv)
{
	int i;
	int nr_gpcs = 1;
	uint64_t entry;
	int fd = open("#c/sysctl", O_RDWR), ret;
	int kfd = -1;
	bool smallkernel = false;
	void * x;
	static char cmd[512];
	if (fd < 0) {
		perror("#c/sysctl");
		exit(1);
	}
	argc--,argv++;
	if (! strcmp(argv[0], "-s")) {
		smallkernel = true;
		argc--,argv++;
	}
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [-s] vmimage entrypoint\n", argv[0]);
		exit(1);
	}
	entry = strtoull(argv[2], 0, 0);
	kfd = open(argv[1], O_RDONLY);
	if (kfd < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (ros_syscall(SYS_setup_vmm, nr_gpcs, 0, 0, 0, 0, 0) != nr_gpcs) {
		perror("Guest pcore setup failed");
		exit(1);
	}
	/* blob that is faulted in from the EPT first.  we need this to be in low
	 * memory (not above the normal mmap_break), so the EPT can look it up.
	 * Note that we won't get 4096.  The min is 1MB now, and ld is there. */
	/* just get 128MiB for now.
	mmap_blob = mmap((int*)4096, 128 * 1048576, PROT_READ | PROT_WRITE,
	                 MAP_ANONYMOUS, -1, 0);
	if (mmap_blob == MAP_FAILED) {
		perror("Unable to mmap");
		exit(1);
	}

	// read in the kernel.
	x = mmap_blob;
	for(;;) {
		amt = read(kfd, x, 1048576);
		if (amt < 0) {
			perror("read");
			exit(1);
		}
		if (amt == 0) {
			break;
		}
		x += amt;
	}

		my_threads = malloc(sizeof(pthread_t) * nr_threads);
		my_retvals = malloc(sizeof(void*) * nr_threads);
		if (!(my_retvals && my_threads))
			perror("Init threads/malloc");

		pthread_can_vcore_request(FALSE);	/* 2LS won't manage vcores */
		pthread_need_tls(FALSE);
		pthread_lib_init();					/* gives us one vcore */
		vcore_request(nr_threads - 1);		/* ghetto incremental interface */
		for (int i = 0; i < nr_threads; i++) {
			x = __procinfo.vcoremap;
			printf("%p\n", __procinfo.vcoremap);
			printf("Vcore %d mapped to pcore %d\n", i,
			    	__procinfo.vcoremap[i].pcoreid);
		}
		if (pthread_create(&my_threads[0], NULL, &talk_thread, NULL))
			perror("pth_create failed");
//		if (pthread_create(&my_threads[1], NULL, &fail, NULL))
//			perror("pth_create failed");
	printf("threads started\n");

	if (0) for (int i = 0; i < nr_threads-1; i++) {
		int ret;
		if (pthread_join(my_threads[i], &my_retvals[i]))
			perror("pth_join failed");
		printf("%d %d\n", i, ret);
	}
	

	ret = syscall(33, 1);
	if (ret < 0) {
		perror("vm setup");
		exit(1);
	}
	p512 = mmap_blob;
	p1 = &p512[512];
	p2m = &p512[1024];
	// Map the top 2G of kernel address space. It's ok to map memory
	// we have no access to. Further, since the PML2 is the same in all cases,
	// two PML3s can point to it.
	// Just assume we get the maximum, and build your kernels accordingly.
#define KERNBASElow 0xffffffff80000000
#define KERNBASEhigh 0xffffffffc0000000
#define KERNBASEsmall 0xfffffffff0000000
#define _2MiB 0x200000
	if (smallkernel) {
		p512[PML4(KERNBASEsmall)] = (unsigned long long)p1 | 7;
		p1[PML3(KERNBASEsmall)] = /*0x87; */(unsigned long long)p2m | 7;
		// to make it easy, we add KERNBASE to entry for you. Less typing.
		entry += KERNBASEsmall;
	} else {
		p512[PML4(KERNBASEhigh)] = (unsigned long long)p1 | 7;
		p1[PML3(KERNBASEhigh)] = /*0x87; */(unsigned long long)p2m | 7;
		p1[PML3(KERNBASElow)] = /*0x87; */(unsigned long long)p2m | 7;
		// to make it easy, we add KERNBASE to entry for you. Less typing.
		entry += KERNBASElow;
	}
	for(i = 0; i < 512; i++)
		p2m[PML2(KERNBASElow) + i * _2MiB] = 0x87 | _2MiB;
			
	printf("p512 %p p512[0] is 0x%lx p1 %p p1[0] is 0x%x\n", p512, p512[0], p1, p1[0]);
	sprintf(cmd, "V 0x%x 0x%x 0x%x", entry, (unsigned long long) &stack[1024], (unsigned long long) p512);
	printf("Writing command :%s:\n", cmd);
	ret = write(fd, cmd, strlen(cmd));
	if (ret != strlen(cmd)) {
		perror(cmd);
	}

	sprintf(cmd, "V 0 0 0");
	while (! done) {
		if (debug)
			fprintf(stderr, "RESUME\n");
		ret = write(fd, cmd, strlen(cmd));
		if (ret != strlen(cmd)) {
			perror(cmd);
		}
	}
	return 0;
}
