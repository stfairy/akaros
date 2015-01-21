sed -i 's/\([^_]\)MAXCPU/\1MAX_NUM_CPUS/g' $*
sed -i 's/curcpu/hw_core_id()/g' $*
sed -i 's/MSR_PAT/MSR_IA32_CR_PAT/g' $*
sed -i 's/CR4_XSAVE/X86_CR4_OSXSAVE/g' $*
sed -i 's/boolean_t/bool/g' $*
