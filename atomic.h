#ifndef _atomic_h

/* This wraps the gcc atomic code. 
   I think this was finally standardized in c11 */

#define atomic_compare_and_swap __sync_val_compare_and_swap
#define atomic_fetch_and_add    __sync_fetch_and_add
#define atomic_sync __sync_synchronize
#endif 

