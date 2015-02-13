/*-
 * Copyright (c) 2014, Neel Natu (neel@freebsd.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/errno.h>

#include <machine/cpufunc.h>
#include <machine/specialreg.h>

#include "svm_msr.h"

#ifndef MSR_AMDK8_IPM
#define	MSR_AMDK8_IPM	0xc0010055
#endif

enum {
	IDX_MSR_LSTAR,
	IDX_MSR_CSTAR,
	IDX_MSR_STAR,
	IDX_MSR_SYSCALL_MASK,
	HOST_MSR_NUM				/* must be the last enumeration */
};

static uint64_t host_msrs[HOST_MSR_NUM];

void svm_msr_init(void)
{
	/* 
	 * It is safe to cache the values of the following MSRs because they
	 * don't change based on hw_core_id(), curproc or curthread.
	 */
	host_msrs[IDX_MSR_LSTAR] = read_msr(MSR_LSTAR);
	host_msrs[IDX_MSR_CSTAR] = read_msr(MSR_CSTAR);
	host_msrs[IDX_MSR_STAR] = read_msr(MSR_STAR);
	host_msrs[IDX_MSR_SYSCALL_MASK] = read_msr(MSR_SYSCALL_MASK);
}

void svm_msr_guest_init(struct svm_softc *sc, int vcpu)
{
	/*
	 * All the MSRs accessible to the guest are either saved/restored by
	 * hardware on every #VMEXIT/VMRUN (e.g., G_PAT) or are saved/restored
	 * by VMSAVE/VMLOAD (e.g., MSR_GS_BASE).
	 *
	 * There are no guest MSRs that are saved/restored "by hand" so nothing
	 * more to do here.
	 */
	return;
}

void svm_msr_guest_enter(struct svm_softc *sc, int vcpu)
{
	/*
	 * Save host MSRs (if any) and restore guest MSRs (if any).
	 */
}

void svm_msr_guest_exit(struct svm_softc *sc, int vcpu)
{
	/*
	 * Save guest MSRs (if any) and restore host MSRs.
	 */
	write_msr(MSR_LSTAR, host_msrs[IDX_MSR_LSTAR]);
	write_msr(MSR_CSTAR, host_msrs[IDX_MSR_CSTAR]);
	write_msr(MSR_STAR, host_msrs[IDX_MSR_STAR]);
	write_msr(MSR_SYSCALL_MASK, host_msrs[IDX_MSR_SYSCALL_MASK]);

	/* MSR_KERNEL_GS_BASE will be restored on the way back to userspace */
}

int
svm_rdmsr(struct svm_softc *sc, int vcpu, unsigned int num,
	  uint64_t * result,
		  bool * retu)
{
	int error = 0;

	switch (num) {
		case MSR_AMDK8_IPM:
			*result = 0;
			break;
		default:
			error = EINVAL;
			break;
	}

	return (error);
}

int
svm_wrmsr(struct svm_softc *sc, int vcpu, unsigned int num, uint64_t val,
	  bool * retu)
{
	int error = 0;

	switch (num) {
		case MSR_AMDK8_IPM:
			/*
			 * Ignore writes to the "Interrupt Pending Message" MSR.
			 */
			break;
		default:
			error = EINVAL;
			break;
	}

	return (error);
}