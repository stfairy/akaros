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

