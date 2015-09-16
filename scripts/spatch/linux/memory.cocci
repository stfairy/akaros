@@
@@
-GFP_ATOMIC
+0

@@
@@
-GFP_KERNEL
+KMALLOC_WAIT

@@
@@
-GFP_WAIT
+KMALLOC_WAIT

@@
@@
-__GFP_WAIT
+KMALLOC_WAIT

@@
expression SZ;
expression FL;
@@
-kzalloc(SZ, FL)
+kzmalloc(SZ, FL)

@@
expression SZ;
expression CNT;
expression FL;
@@
-kcalloc(CNT, SZ, FL)
+kzmalloc((CNT) * (SZ), FL)

// These are dangerous in Akaros.  If someone is using a page* for multiple
// pages, it's probably okay, but we should look closely.
@@
expression ADDR;
expression ORDER;
@@
-__free_pages(ADDR, ORDER)
+free_cont_pages(page2kva(ADDR), ORDER)

@@
expression FLAGS;
expression ORDER;
@@
-alloc_pages(FLAGS, ORDER)
+kva2page(get_cont_pages(ORDER, FLAGS))

@@
expression FLAGS;
@@
-__get_free_page(FLAGS)
+kpage_alloc_addr()

@@
expression PG;
@@
-get_page(PG)
+page_incref(PG)

@@
expression PG;
@@
-put_page(PG)
+page_decref(PG)

@@
struct page *page;
@@
-page->_count
+page->pg_kref.refcount

@@
expression KVA;
@@
-virt_to_head_page(KVA)
+kva2page(KVA)

@@
expression KVA;
@@
-virt_to_page(KVA)
+kva2page(KVA)

@@
expression KVA;
@@
-virt_to_bus(KVA)
+PADDR(KVA)
