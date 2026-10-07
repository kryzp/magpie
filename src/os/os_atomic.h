#ifndef OS_ATOMIC_H
#define OS_ATOMIC_H

typedef enum OS_MemoryOrder
{
    OS_MemoryOrder_Relaxed,
    OS_MemoryOrder_Consume,
    OS_MemoryOrder_Acquire,
    OS_MemoryOrder_Release,
    OS_MemoryOrder_AcqRel,
    OS_MemoryOrder_SeqCst,
	OS_MemoryOrder_COUNT
}
OS_MemoryOrder;

#endif // OS_ATOMIC_H
