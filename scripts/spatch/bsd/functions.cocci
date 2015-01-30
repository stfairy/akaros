@@
@@
-load_cr4(
+lcr4(
 ...)

@@
@@
-disable_intr()
+disable_irq()

@@
@@
-enable_intr()
+enable_irq()

@@
expression E;
@@
-atomic_readandclear_long(
+atomic_swap(
E
+ , 0
 )

@@
@@
- atomic_cmpset_long(
+ atomic_cas(
 ...)

@@
@@
- atomic_set_long(
+ atomic_set(
 ...)

@@
@@
- flsl(
+ fls64(
 ...)

@ print@
@@
-printf(
+printk(
...)

@@
@@
-fpusave(
+save_fp_state(
...)

@@
@@
-fpurestore(
+restore_fp_state(
...)

@@
@@
-fpu_save_area_free(
+kfree(
...)

@@
@@
-fpu_save_area_alloc()
+kmalloc(sizeof(struct ancillary_state), KMALLOC_WAIT)

@@
	expression E;
@@
-fpu_save_area_reset(
+memmove(
E
+ , &x86_default_fpu, sizeof(x86_default_fpu)
 )

@@
@@
-min(
+MIN(
 ...)
