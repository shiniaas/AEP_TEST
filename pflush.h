#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
	cacheline flush:
	if your cpu does't support clwb (you can check it by command 'cat /proc/cpuinfo'),
	you can replace it with clflush or clflushopt
*/
#define asm_flush(addr)						\
({								\
	__asm__ __volatile__ ("clwb %0" : : "m"(*addr));	\
})

/*
	memory fence:
	sfence can be replaced with mfence or lfence
*/
#define asm_fence()					\
({							\
	__asm__ __volatile__ ("sfence":::"memory");	\
})

