#ifndef _atomic_h

/* This wraps the gcc atomic code. 
   I think this was finally standardized in c11 */

#define atomic_compare_and_swap __sync_val_compare_and_swap
#define atomic_sub(a,b)		__sync_sub_and_fetch(a,b)
#define atomic_add(a,b)		__sync_add_and_fetch(a,b)
#define atomic_sync		__sync_synchronize
#endif 

