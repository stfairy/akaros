@ rulem @
expression E1;
@@

- malloc(E1, 
+ kzmalloc(E1, KERN_WAIT
-  ...   ) 
+ )

@ rulef @
expression E1;
@@

- free(E1, 
+ kfree(E1
-  ...   ) 
+ )

