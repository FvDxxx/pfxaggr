#ifndef PFX_TREE_H
#define PFX_TREE_H

#include "pfx_addr.h"

typedef struct _pfx_node {
	pfx_ipaddr_t ip;
	pfx_ipaddr_t mi;
	unsigned char  mask;
	unsigned char  flags;
	pfx_ipaddr_t am;
	struct _pfx_node *left;
	struct _pfx_node *right;
} pfx_node_t;

typedef struct {
	pfx_node_t *v4;
	unsigned long v4size;
	pfx_node_t *v6;
	unsigned long v6size;
} pfx_tree_t;

pfx_tree_t* pfx_tree_new();
void pfx_tree_destroy(pfx_tree_t *tree);
unsigned long pfx_tree_size(pfx_tree_t *tree, pfx_addr_t v4v6);
int  pfx_tree_insert(pfx_tree_t *tree, const pfx_ipaddr_t newnet, const unsigned char newmask);
void pfx_tree_iter(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, void *calldata), void *itercalldata);
void pfx_tree_iteraggr0(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, void *calldata), void *iterdata);
void pfx_tree_iteraggr1(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, void *calldata), void *iterdata);
void pfx_tree_iteraggr2(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, unsigned char maskupto, void *calldata), void *iterdata);
void pfx_tree_iteraggr3(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, unsigned char mask1, unsigned char mask2, void *calldata), void *iterdata);
void pfx_tree_iteraggr4(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t addr, unsigned char mask, unsigned char mask1, unsigned char mask2, void *calldata), void *iterdata);

#endif
