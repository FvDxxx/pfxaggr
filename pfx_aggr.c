
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "pfx_tree.h"



static void itercnt(pfx_ipaddr_t net, unsigned char mask, void *calldata) {
	*((unsigned long *)calldata) += 1;
}
static void itercnt2(pfx_ipaddr_t net, unsigned char mask, unsigned char upto, void *calldata) {
	*((unsigned long *)calldata) += 1;
}

static void itercnt3(pfx_ipaddr_t net, unsigned char mask, unsigned char from, unsigned char to, void *calldata) { 
	*((unsigned long *)calldata) += 1;
}


static void iter(pfx_ipaddr_t net, unsigned char mask, void *calldata) {
	char str[INET6_ADDRSTRLEN];
	pfx_addr2str(net, str, INET6_ADDRSTRLEN);
	fprintf(stdout, "%s/%d\n", str, mask);
	*((unsigned long *)calldata) += 1;
}

static void iter2(pfx_ipaddr_t net, unsigned char mask, unsigned char upto, void *calldata) {
	char str[INET6_ADDRSTRLEN];
	pfx_addr2str(net, str, INET6_ADDRSTRLEN);
	if (mask!=upto && upto !=255) {
		fprintf(stdout, "%s/%d-%d\n", str, mask, upto);
	} else {
		fprintf(stdout, "%s/%d\n", str, mask);
	}
	*((unsigned long *)calldata) += 1;
}

static void iter3(pfx_ipaddr_t net, unsigned char mask, unsigned char from, unsigned char to, void *calldata) {
	char str[INET6_ADDRSTRLEN];
	pfx_addr2str(net, str, INET6_ADDRSTRLEN);
	//fprintf(stderr, "iter3: %s, %d, %d, %d\n", str, mask, from, to);
	if (to==255) {
		if (mask!=from && from!=255) {
			fprintf(stdout, "%s/%d-%d\n", str, mask, from);
		} else {
			fprintf(stdout, "%s/%d\n", str, mask);
		}
	} else {
		fprintf(stdout, "%s/%d,/%d-%d\n", str, mask, from, to);
	} 
	*((unsigned long *)calldata) += 1;
}

#define STRINGSIZE 160
int main(int argc, char *argv[]) {
	// not nice, it's just a hack for testing
	char s[STRINGSIZE];
	//char *sptr;
	unsigned long pfx_count;
	int aggrmode;
	int verbosemode;
	int commentmode;
	int i;
	
	pfx_tree_t *tree;
	
	tree = pfx_tree_new();

	aggrmode = 1;
	verbosemode = 0;
	commentmode = 0;
	
	for (i=1; i<argc; i++) {
		if (strcmp(argv[i], "-na")==0) {
			aggrmode = -1;
		} else if (strcmp(argv[i], "-a0")==0) {
			aggrmode = 0;
		} else if (strcmp(argv[i], "-a1")==0) {
			aggrmode = 1;
		} else if (strcmp(argv[i], "-a2")==0) {
			aggrmode = 2;
		} else if (strcmp(argv[i], "-a3")==0) {
			aggrmode = 3;
		} else if (strcmp(argv[i], "-a4")==0) {
			aggrmode = 4;
		} else if (strcmp(argv[i], "-v")==0) {
			verbosemode++;
		} else if (strcmp(argv[i], "-c")==0) {
			commentmode = 1;
		} else {
			fprintf(stderr, "%s: [-v|-a0|-a1|-a2|-a3|-a4]\n", argv[0]);
			fprintf(stderr, "Read prefixes (v4 or v6) from stdin and aggregated them.\n");
			fprintf(stderr, " -v      complain on stderr about unparsable input\n");
			fprintf(stderr, " -v -v   not only complain, but also warn about unaligned nets\n");
			fprintf(stderr, " -c      start output with comment on result\n");
			fprintf(stderr, " -na     no aggregation (just parse, align, unique and sort)\n");
			fprintf(stderr, " -a0     report only least specific prefixes of input\n");
			fprintf(stderr, " -a1     report least specific prefixes (eventually artificial)\n");
			fprintf(stderr, " -a2     report prefix-length sequences from input as pfx/START-END\n");
			fprintf(stderr, " -a3     in addition to -a2, also report \"complete\" sets of more\n");
			fprintf(stderr, "         specific prefix-length sequences as pfx/MASK,/START-END;\n");
			fprintf(stderr, "         pfx/MASK is NOT part of the input, unless reported somewhere\n");
			fprintf(stderr, "         else; no pfx/mask from input will be reported twice; not\n");
			fprintf(stderr, "         suitable as base for Juniper route-filter\n");
			fprintf(stderr, " -a4     similar to -a3, but ok as input for Juniper route-filters;\n");
			fprintf(stderr, "         complete set of prefix-length sequences will be only reported\n");
			fprintf(stderr, "         \"relative\" to the closest by less specific prefix of the input.\n");
			fprintf(stderr, "\n");
			exit(1);
		}
	}
	
	
	while (fgets(s, STRINGSIZE, stdin)) {
		pfx_ipaddr_t newaddr, newnet;
		unsigned char mask;

		if (!pfx_str2addr(&newaddr, &mask, s)) {
			if (verbosemode>=1) {
				fprintf(stderr, "Error converting str to addr %s\n", s);
			}
			continue;
		}
		if (!pfx_masknet(newaddr, mask, &newnet)) {
			char str1[INET6_ADDRSTRLEN], str2[INET6_ADDRSTRLEN];
			pfx_addr2str(newaddr, str1, INET6_ADDRSTRLEN);
			pfx_addr2str(newnet,  str2, INET6_ADDRSTRLEN);
			if (verbosemode>=2) {
				fprintf(stderr, "Unaligned network, before=%s/%d, after=%s/%d\n", str1, mask, str2, mask);
			}
		}
		pfx_tree_insert(tree, newnet, mask);
	}
	
	if (commentmode) {
		pfx_count = 0;
		pfx_tree_iter(tree, v4, itercnt, &pfx_count);
		fprintf(stdout, "# Input v4: %lu unique prefixes\n", pfx_count);
		pfx_count = 0;
		pfx_tree_iter(tree, v6, itercnt, &pfx_count);
		fprintf(stdout, "# Input v6: %lu unique prefixes\n", pfx_count);
	}
	switch (aggrmode) {
	case -1:
		if (commentmode) {
			fprintf(stdout, "# no aggr mode\n");
		}
		pfx_tree_iter(tree, v4, iter, &pfx_count);
		pfx_tree_iter(tree, v6, iter, &pfx_count);
		break;
	case 0:	
		if (commentmode) {
			fprintf(stdout, "# aggr0 mode\n");
			pfx_count = 0;
			pfx_tree_iteraggr0(tree, v4, itercnt, &pfx_count);
			fprintf(stdout, "# Output v4: %lu unique prefixes\n", pfx_count);
			pfx_count = 0;
			pfx_tree_iteraggr0(tree, v6, itercnt, &pfx_count);
			fprintf(stdout, "# Output v6: %lu unique prefixes\n", pfx_count);
		}
		pfx_tree_iteraggr0(tree, v4, iter, &pfx_count);
		pfx_tree_iteraggr0(tree, v6, iter, &pfx_count);
		break;
	case 1:	
		if (commentmode) {
			fprintf(stdout, "# aggr1 mode\n");
			pfx_count = 0;
			pfx_tree_iteraggr1(tree, v4, itercnt, &pfx_count);
			fprintf(stdout, "# Output v4: %lu unique prefixes\n", pfx_count);
			pfx_count = 0;
			pfx_tree_iteraggr1(tree, v6, itercnt, &pfx_count);
			fprintf(stdout, "# Output v6: %lu unique prefixes\n", pfx_count);
		}
		pfx_tree_iteraggr1(tree, v4, iter, &pfx_count);
		pfx_tree_iteraggr1(tree, v6, iter, &pfx_count);
		break;
	case 2:	
		if (commentmode) {
			fprintf(stdout, "# aggr2 mode\n");
			pfx_count = 0;
			pfx_tree_iteraggr2(tree, v4, itercnt2, &pfx_count);
			fprintf(stdout, "# Output v4: %lu unique prefixes\n", pfx_count);
			pfx_count = 0;
			pfx_tree_iteraggr2(tree, v6, itercnt2, &pfx_count);
			fprintf(stdout, "# Output v6: %lu unique prefixes\n", pfx_count);
		}
		pfx_tree_iteraggr2(tree, v4, iter2, &pfx_count);
		pfx_tree_iteraggr2(tree, v6, iter2, &pfx_count);
		break;
	case 3:	
		if (commentmode) {
			fprintf(stdout, "# aggr3 mode\n");
			pfx_count = 0;
			pfx_tree_iteraggr3(tree, v4, itercnt3, &pfx_count);
			fprintf(stdout, "# Output v4: %lu unique prefixes\n", pfx_count);
			pfx_count = 0;
			pfx_tree_iteraggr3(tree, v6, itercnt3, &pfx_count);
			fprintf(stdout, "# Output v6: %lu unique prefixes\n", pfx_count);
		}
		pfx_tree_iteraggr3(tree, v4, iter3, &pfx_count);
		pfx_tree_iteraggr3(tree, v6, iter3, &pfx_count);
		break;
	case 4:	
		if (commentmode) {
			fprintf(stdout, "# aggr4 mode\n");
			pfx_count = 0;
			pfx_tree_iteraggr4(tree, v4, itercnt3, &pfx_count);
			fprintf(stdout, "# Output v4: %lu unique prefixes\n", pfx_count);
			pfx_count = 0;
			pfx_tree_iteraggr4(tree, v6, itercnt3, &pfx_count);
			fprintf(stdout, "# Output v6: %lu unique prefixes\n", pfx_count);
		}
		pfx_tree_iteraggr4(tree, v4, iter3, &pfx_count);
		pfx_tree_iteraggr4(tree, v6, iter3, &pfx_count);
		break;
	}

	exit(0);
}
