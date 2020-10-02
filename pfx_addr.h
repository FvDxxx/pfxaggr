#ifndef PFX_ADDR_H
#define PFX_ADDR_H

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t pfx_v4addr_t;

typedef struct {
	uint64_t h;
	uint64_t l;
} pfx_v6addr_t;

typedef enum { v4, v6 } pfx_addr_t;

// By using union we blow up size requirements
// for the v4 tree and make overall code slower
// due to v4/v6 check - but we make the code some-
// what more simplified (rather than duplicate 
// almost similar code everywhere

typedef struct {
	pfx_addr_t addrtype;
	union addr {
		pfx_v4addr_t v4;
		pfx_v6addr_t v6;
	} addr;
} pfx_ipaddr_t;


extern pfx_ipaddr_t pfx_v4masks[33];
extern pfx_ipaddr_t pfx_v6masks[129];
extern pfx_ipaddr_t pfx_v4middleadds[33];
extern pfx_ipaddr_t pfx_v6middleadds[129];
extern pfx_ipaddr_t pfx_v4bits[33];
extern pfx_ipaddr_t pfx_v6bits[129];


void pfx_addr2str(pfx_ipaddr_t addr, char* str, ssize_t strlen);
int  pfx_str2addr(pfx_ipaddr_t *addr, unsigned char *mask, char *str);
int pfx_masknet(pfx_ipaddr_t addr, unsigned char mask, pfx_ipaddr_t *netaddr);


#endif
