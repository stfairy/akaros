sed -i 's/\([^_]\)MAXCPU/\1MAX_NUM_CPUS/g' $*
sed -i 's/curcpu/hw_core_id()/g' $*
sed -i 's/MSR_PAT/MSR_IA32_CR_PAT/g' $*
sed -i 's/CR4_XSAVE/X86_CR4_OSXSAVE/g' $*
sed -i 's/boolean_t/bool/g' $*
sed -i 's/MSR_VMX_BASIC/MSR_IA32_VMX_BASIC/g' $*
sed -i 's/MSR_VMX_PROCBASED_CTLS/MSR_IA32_VMX_PROCBASED_CTLS/g' $*
sed -i 's/MSR_VMX_TRUE_PROCBASED_CTLS/MSR_IA32_VMX_TRUE_PROCBASED_CTLS/g' $*
sed -i 's/MSR_VMX_PROCBASED_CTLS2/MSR_IA32_VMX_PROCBASED_CTLS2/g' $*
sed -i 's/MSR_VMX_PINBASED_CTLS/MSR_IA32_VMX_PINBASED_CTLS/g' $*
sed -i 's/MSR_VMX_TRUE_PINBASED_CTLS/MSR_IA32_VMX_TRUE_PINBASED_CTLS/g' $*
sed -i 's/MSR_VMX_EXIT_CTLS/MSR_IA32_VMX_EXIT_CTLS/g' $*
sed -i 's/MSR_VMX_TRUE_EXIT_CTLS/MSR_IA32_VMX_TRUE_EXIT_CTLS/g' $*
sed -i 's/MSR_VMX_ENTRY_CTLS/MSR_IA32_VMX_ENTRY_CTLS/g' $*
sed -i 's/MSR_VMX_TRUE_ENTRY_CTLS/MSR_IA32_VMX_TRUE_ENTRY_CTLS/g' $*
sed -i 's/MSR_VMX_CR0_FIXED0/MSR_IA32_VMX_CR0_FIXED0/g' $*
sed -i 's/MSR_VMX_CR0_FIXED1/MSR_IA32_VMX_CR0_FIXED1/g' $*
sed -i 's/MSR_VMX_CR4_FIXED0/MSR_IA32_VMX_CR4_FIXED0/g' $*
sed -i 's/MSR_VMX_CR4_FIXED1/MSR_IA32_VMX_CR4_FIXED1/g' $*
sed -i 's/IA32_FEATURE_CONTROL_LOCK/FEATURE_CONTROL_LOCKED/g' $*
sed -i 's/IA32_FEATURE_CONTROL_VMX_EN/FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX/g' $*
sed -i 's/IDT_BP/T_BRKPT/g' $*
sed -i 's/IDT_OF/T_OFLOW/g' $*
sed -i 's/IDT_BR/T_BOUND/g' $*
sed -i 's/IDT_MC/T_MCHK/g' $*
sed -i 's/IDT_NMI/T_NMI/g' $*

sed -i 's/IDT_PF/T_PGFLT/g' $*
sed -i 's/IDT_DF/T_DBLFLT/g' $*
sed -i 's/IDT_DE/T_DIVIDE/g' $*
sed -i 's/IDT_TS/T_TSS/g' $*
sed -i 's/IDT_SS/T_STACK/g' $*
sed -i 's/IDT_NP/T_SEGNP/g' $*
sed -i 's/IDT_GP/T_GPFLT/g' $*

sed -i 's/PSL_I/FL_IF/g' $*

sed -i 's/VM_PROT_READ/PROT_READ/g' $*
sed -i 's/VM_PROT_WRITE/PROT_WRITE/g' $*
sed -i 's/VM_PROT_EXECUTE/PROT_EXEC/g' $*


sed -i 's/MSR_GSBASE/MSR_GS_BASE/g' $*
sed -i 's/MSR_FSBASE/MSR_FS_BASE/g' $*
sed -i 's/MSR_KGSBASE/MSR_KERNEL_GS_BASE/g' $*
sed -i 's/MSR_SYSENTER_CS_MSR/MSR_IA32_SYSENTER_CS/g' $*
sed -i 's/MSR_SYSENTER_ESP_MSR/MSR_IA32_SYSENTER_ESP/g' $*
sed -i 's/MSR_SYSENTER_EIP_MSR/MSR_IA32_SYSENTER_EIP/g' $*
sed -i 's/MSR_EFER/MSR_EFER/g' $*
sed -i 's/MSR_TSC/MSR_IA32_TSC/g' $*
sed -i 's/MSR_SF_MASK/MSR_SYSCALL_MASK/g' $*
sed -i 's/DEFAULT_APIC_BASE/LAPIC_BASE/g' $*
sed -i 's/MSR_VMX_EPT_VPID_CAP/MSR_IA32_VMX_EPT_VPID_CAP/g' $*
sed -i 's/MSR_PLATFORM_INFO/MSR_NHM_PLATFORM_INFO/g' $*
sed -i 's/MSR_TURBO_RATIO_LIMIT/MSR_NHM_TURBO_RATIO_LIMIT/g' $*
sed -i 's/\([^\.]\)tsc_freq/\1system_timing.tsc_freq/g' $*
sed -i 's/struct  *mtx/qlock_t/g' $*
sed -i 's/struct  *savefpu/struct ancillary_state/g' $*

sed -i 's/MSR_APICBASE/MSR_IA32_APICBASE/g' $*
# cocci just can't do it. Don't know why.

sed -i 's/volatile cpuset_t/checklist_mask_t/g' $*
sed -i 's/cpuset_t/checklist_mask_t/g' $*

sed -i 's/struct  *seg_desc[ ,]/syssegdesc_t/g' $*