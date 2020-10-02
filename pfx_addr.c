
#include "pfx_addr.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

void pfx_addr2str(pfx_ipaddr_t addr, char* str, ssize_t strlen) {
	if (addr.addrtype == v4) {
		snprintf(str, strlen, "%d.%d.%d.%d",
			(addr.addr.v4&0xff000000U)>>24, \
			(addr.addr.v4&0xff0000U)>>16, \
			(addr.addr.v4&0xff00U)>>8, \
			(addr.addr.v4&0xffU));
	} else {
		struct sockaddr_in6 sa;
		
		sa.sin6_addr.s6_addr[0]  = (addr.addr.v6.h >> 56) & 0xff;
		sa.sin6_addr.s6_addr[1]  = (addr.addr.v6.h >> 48) & 0xff;
		sa.sin6_addr.s6_addr[2]  = (addr.addr.v6.h >> 40) & 0xff;
		sa.sin6_addr.s6_addr[3]  = (addr.addr.v6.h >> 32) & 0xff;
		sa.sin6_addr.s6_addr[4]  = (addr.addr.v6.h >> 24) & 0xff;
		sa.sin6_addr.s6_addr[5]  = (addr.addr.v6.h >> 16) & 0xff;
		sa.sin6_addr.s6_addr[6]  = (addr.addr.v6.h >>  8) & 0xff;
		sa.sin6_addr.s6_addr[7]  = (addr.addr.v6.h >>  0) & 0xff;
		sa.sin6_addr.s6_addr[8]  = (addr.addr.v6.l >> 56) & 0xff;
		sa.sin6_addr.s6_addr[9]  = (addr.addr.v6.l >> 48) & 0xff;
		sa.sin6_addr.s6_addr[10] = (addr.addr.v6.l >> 40) & 0xff;
		sa.sin6_addr.s6_addr[11] = (addr.addr.v6.l >> 32) & 0xff;
		sa.sin6_addr.s6_addr[12] = (addr.addr.v6.l >> 24) & 0xff;
		sa.sin6_addr.s6_addr[13] = (addr.addr.v6.l >> 16) & 0xff;
		sa.sin6_addr.s6_addr[14] = (addr.addr.v6.l >>  8) & 0xff;
		sa.sin6_addr.s6_addr[15] = (addr.addr.v6.l >>  0) & 0xff;
		
		inet_ntop(AF_INET6, &(sa.sin6_addr), str, strlen);
	}
}


// this does temp change str!!
int pfx_str2addr(pfx_ipaddr_t *addr, unsigned char *mask, char* str) {
	char *sptr;
	int retval;
	char savedchar;
	
	if (!str) {
		return 0;
	}
	sptr = str;
	while (*sptr && *sptr != '.' && *sptr != ':' && ((*sptr>='0'&&*sptr<='9') || (*sptr>='a'&&*sptr<='f') || (*sptr>='A'&&*sptr<='F'))) {
		sptr++;
	}
	if (*sptr==':') { // seeing a : we consider this as an IPv6 address in any case
		// cut of temporary "junk" at the end
		struct sockaddr_in6 sa;

		while (*sptr && (*sptr == ':' || *sptr == '.' || (*sptr>='0'&&*sptr<='9') || (*sptr>='a'&&*sptr<='f') || (*sptr>='A'&&*sptr<='F'))) {
			sptr++;
		}
		if (*sptr!='/') {
			*mask = 128;
		} else {
			// should we check on sanity here? 0<=mask<=128?
			*mask = atoi(sptr+1);
		}
		savedchar = *sptr;
		*sptr = 0;
//fprintf(stderr, "calling inet_pton with %s\n", str);
		retval = inet_pton(AF_INET6, str, &(sa.sin6_addr));
		addr->addrtype  = v6;
		addr->addr.v6.h =
			(( (unsigned long long)(sa.sin6_addr.s6_addr[0]))  <<56) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[1]))  <<48) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[2]))  <<40) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[3]))  <<32) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[4]))  <<24) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[5]))  <<16) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[6]))  <<8 ) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[7]))  <<0 );
		addr->addr.v6.l =
			(( (unsigned long long)(sa.sin6_addr.s6_addr[8]))  <<56) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[9]))  <<48) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[10])) <<40) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[11])) <<32) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[12])) <<24) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[13])) <<16) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[14])) <<8 ) +
			(( (unsigned long long)(sa.sin6_addr.s6_addr[15])) <<0);
	} else { 
		// either it's just a number or we've seen the end or a dot ... IPv4 ...
		// ok, we do not allow by this octal or hex stype IPv4 address presentation
		struct sockaddr_in sa;
		while (*sptr && (*sptr == '.' || (*sptr>='0'&&*sptr<='9'))) {
			sptr++;
		}
		if (*sptr!='/') {
			*mask = 32;
		} else {
			// should we check on sanity here? 0<=mask<=32?
			*mask = atoi(sptr+1);
		}
		savedchar = *sptr;
		*sptr = 0;
		// Yes, inet_aton is depreciated, because it's limited to v4; but
		// unlike inet_pton it can parse 10 or 10.5 as valid IPv4 addr
//fprintf(stderr, "calling inet_aton with %s\n", str);
		retval = inet_aton(str, &(sa.sin_addr));
		addr->addrtype = v4;
		addr->addr.v4  = 
			(((unsigned char*) &(sa.sin_addr))[0]<<24) +
			(((unsigned char*) &(sa.sin_addr))[1]<<16) +
			(((unsigned char*) &(sa.sin_addr))[2]<<8) +
			(((unsigned char*) &(sa.sin_addr))[3]<<0); 
	}
	*sptr = savedchar;
	return retval;
}

int pfx_masknet(pfx_ipaddr_t net, unsigned char mask, pfx_ipaddr_t *newnet) {
	// we assume the caller gave us equal addr types
	newnet->addrtype = net.addrtype;
	if (newnet->addrtype == v4) {
		newnet->addr.v4 = net.addr.v4 & pfx_v4masks[mask].addr.v4;
		return (newnet->addr.v4==net.addr.v4);
	} else {
		newnet->addr.v6.h = net.addr.v6.h & pfx_v6masks[mask].addr.v6.h;
		newnet->addr.v6.l = net.addr.v6.l & pfx_v6masks[mask].addr.v6.l;
		return (newnet->addr.v6.h==net.addr.v6.h && newnet->addr.v6.l==net.addr.v6.l);
	}
}


pfx_ipaddr_t pfx_v4masks[33];
pfx_ipaddr_t pfx_v6masks[129];
pfx_ipaddr_t pfx_v4middleadds[33];
pfx_ipaddr_t pfx_v6middleadds[129];
pfx_ipaddr_t pfx_v4bits[33];
pfx_ipaddr_t pfx_v6bits[129];

// If your compiler doesn't support construct attribute, then
// remove this and call pfx_addr_init at beginning of main;
static int pfx_addr_isinit = 0;

__attribute__((constructor))
void pfx_addr_init() {
	if (!pfx_addr_isinit) {
		int i;
		
		pfx_addr_isinit=1;
		
		for (i=0; i<33; i++) {
			pfx_v4masks[i].addrtype   = v4;
			pfx_v4middleadds[i].addrtype = v4;
			pfx_v4bits[i].addrtype    = v4;
			pfx_v4masks[i].addr.v4    = (0xffffffffULL<<(32-i))&0xffffffffUL;
			pfx_v4middleadds[i].addr.v4  = (0x100000000ULL>>(i+1));
			pfx_v4bits[i].addr.v4     = ((i)?(1UL<<(i-1)):0);
		}
		for (i=0; i<129; i++) {
			pfx_v6masks[i].addrtype   = v6;
			pfx_v6middleadds[i].addrtype = v6;
			pfx_v6bits[i].addrtype    = v6;

			pfx_v6masks[i].addr.v6.h = ((i<64)?((i==0)?0:(0xffffffffffffffffULL<<(64-i))):0xffffffffffffffffULL);
			pfx_v6masks[i].addr.v6.l = ((i<64)?(0x0000000000000000ULL)        :((i==64)?0:(0xffffffffffffffffULL<<(64-(i-64)))));
			
			pfx_v6middleadds[i].addr.v6.h = ((i<64)?(1ULL<<(63-i)):0);
			pfx_v6middleadds[i].addr.v6.l = ((i<64)?0            :((i<128)?(1ULL<<(128-i-1)):0));
			
			pfx_v6bits[i].addr.v6.h    = ((i>64)?(1ULL<<(i-64-1)):0);
			pfx_v6bits[i].addr.v6.l    = ((i>0&&i<=64)?(1ULL<<(i-1)):0);
		}
	}
}
