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

@would_rather_panic@
@@
-ipi_cpu(
+send_ipi(
  ...)

@rulebcopy@
identifier f;
type T;
	expression FROM, TO, SIZE;
@@
T f(...){<...
bcopy(FROM, TO, SIZE);
...>}
@@
identifier rulebcopy.f;
	expression FROM, TO, SIZE;
@@

- bcopy(FROM, TO, SIZE
+ memmove(TO, FROM, SIZE
   )

@rulebzero@
identifier f;
type T;
	expression TO, SIZE;
@@
T f(...){<...
bzero(TO, SIZE);
...>}
@@
identifier rulebzero.f;
	expression TO, SIZE;
@@
- bzero(TO, SIZE
+ memset(TO, 0,  SIZE
   )

@@
@@
-vm_init(
+virt_init(
...)

@@
expression TO, FROM;
@@
-strcpy(
+ strncpy(
TO, FROM
+ , sizeof(TO)
 )

