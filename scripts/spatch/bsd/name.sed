sed -i 's/\([^_]\)MAXCPU/\1MAX_NUM_CPUS/g' $*
sed -i 's/curcpu/hw_core_id()/g' $*
