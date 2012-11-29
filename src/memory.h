#define disable_interrupts() asm volatile ("cpsid i")
#define enable_interrupts()  asm volatile ("cpsie i")
#define wait_for_interrupt() asm volatile ("wfi")
#define memory_barrier()     asm volatile ("dmb" ::: "memory")
