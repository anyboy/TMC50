/* m32tlb.h : MIPS32 MMU definitions*/
 

#ifndef _M32TLB_H_
#define _M32TLB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*#include <mips/r4ktlb.h> ########## by jason 2005-9-21*/
#include "r4ktlb.h"

/* MIPS32r2 EntryLo extended page frame address bits */
#if __mips == 64 || __mips64
#define TLB_PFNXMASK	0x007fffffc0000000LL
#else
#define TLB_PFNXMASK	0xc0000000
#endif
#define TLB_PFNXSHIFT	29

#ifdef __cplusplus
}
#endif
#endif /* _M32TLB_H_ */

