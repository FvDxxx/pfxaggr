
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
//#include <malloc.h>

#include "pfx_tree.h"


#define PFXNODE_USED  (1<<0)
#define PFXNODE_AGGR1 (1<<1)
#define PFXNODE_AGGR2 (1<<2)
#define PFXNODE_AGGR3 (1<<3)


static void add128(pfx_ipaddr_t a, pfx_ipaddr_t b, pfx_ipaddr_t *dst) {
	// we blindly add, assuming the called knows, those addr are v6
	dst->addr.v6.l = a.addr.v6.l + b.addr.v6.l;
	if (a.addr.v6.l&b.addr.v6.l&(1ULL<<63)) {
		dst->addr.v6.h = a.addr.v6.h + b.addr.v6.h + 1ULL;
	} else {
		dst->addr.v6.h = a.addr.v6.h + b.addr.v6.h;
	}
}

pfx_tree_t *pfx_tree_new() {
	pfx_tree_t *newtree;
	
	newtree = (pfx_tree_t*) malloc(sizeof(pfx_tree_t));
	if (!newtree) {
		fprintf(stderr, "pfx_tree_new: Malloc failure (1)!\n");
		exit(2);
	}
	newtree->v4 = (pfx_node_t*) malloc(sizeof(pfx_node_t));
	newtree->v6 = (pfx_node_t*) malloc(sizeof(pfx_node_t));
	if (!newtree->v4 || !newtree->v6) {
		fprintf(stderr, "pfx_tree_new: Malloc failure (2)!\n");
		exit(2);
	}
	newtree->v4size           = 0;
	newtree->v4->ip.addrtype  = v4;
	newtree->v4->ip.addr.v4   = 0;
	newtree->v4->mi.addrtype  = v4;
	newtree->v4->mi.addr.v4   = 0 + pfx_v4middleadds[0].addr.v4;
	newtree->v4->mask         = 0;
	newtree->v4->flags        = 0;
	newtree->v4->am.addrtype  = v4;
	newtree->v4->left         = 0;
	newtree->v4->right        = 0;

	newtree->v6size           = 0;
	newtree->v6->ip.addrtype = v6;
	newtree->v6->ip.addr.v6.h = 0;
	newtree->v6->ip.addr.v6.l = 0;
	newtree->v6->mi.addrtype = v6;
	newtree->v6->mi.addr.v6.h = 0 + pfx_v6middleadds[0].addr.v6.h;
	newtree->v6->mi.addr.v6.l = 0 + pfx_v6middleadds[0].addr.v6.l;
	newtree->v6->mask         = 0;
	newtree->v6->flags        = 0;
	newtree->v6->am.addrtype  = v6;
	newtree->v6->am.addr.v6.h = 0;
	newtree->v6->am.addr.v6.l = 0;
	newtree->v6->left         = 0;
	newtree->v6->right        = 0;

	return newtree;
}

static void pfx_tree_subdestroy(pfx_node_t *node) {
	if (node) {
		pfx_tree_subdestroy(node->left);
		pfx_tree_subdestroy(node->right);
		free(node);
	}
}

void pfx_tree_destroy(pfx_tree_t *tree) {
	if (tree) { 
		pfx_tree_subdestroy(tree->v4);
		pfx_tree_subdestroy(tree->v6);
		free(tree);
		return;
	}
}

unsigned long pfx_tree_size(pfx_tree_t *tree, pfx_addr_t v4v6) {
	if (!tree) {
		return 0;
	}
	if (v4v6==v4) {
		return tree->v4size;
	} else {
		return tree->v6size;
	}
}

static int pfx_tree_subinsert(pfx_node_t *tree, const pfx_ipaddr_t newnet, const unsigned char newmask) {
	// we are almost sure, tree is always != 0 -- and we assume the caller gave us the "right" tree type
	// we assume the caller properly called us with a v4 tree, otherwise strange things might happen

//{
//char str1[INET6_ADDRSTRLEN], str2[INET6_ADDRSTRLEN], str3[INET6_ADDRSTRLEN];
//pfx_addr2str(newnet, str1, INET6_ADDRSTRLEN);
//pfx_addr2str(tree->ip, str2, INET6_ADDRSTRLEN);
//pfx_addr2str(tree->mi, str3, INET6_ADDRSTRLEN);
//fprintf(stderr, "Insert of %s/%d, treenode %s/%d, middle %s\n", str1, newmask, str2, tree->mask, str3);
//}

	if ( tree->mask == newmask &&
		 ((newnet.addrtype == v4 && tree->ip.addr.v4 == newnet.addr.v4) ||
		  (newnet.addrtype == v6 && tree->ip.addr.v6.h == newnet.addr.v6.h && tree->ip.addr.v6.l == newnet.addr.v6.l)) ) {
		int retval;
		retval = (tree->flags&PFXNODE_USED)?0:1;
		tree->flags = tree->flags | PFXNODE_USED;
		return retval;
	}
	if ((tree->ip.addrtype == v4 && tree->mask==32) || (tree->ip.addrtype == v6 && tree->mask==128)) {
		fprintf(stderr, "pfx_tree_subinsert: Max level reached (%d), something is wrong.\n", tree->mask);
		return 0;
	}
	

	if ( (newnet.addrtype == v4 && (tree->mi.addr.v4 > newnet.addr.v4)) ||
		 (newnet.addrtype == v6 && ((tree->mi.addr.v6.h > newnet.addr.v6.h) || (tree->mi.addr.v6.h == newnet.addr.v6.h && tree->mi.addr.v6.l > newnet.addr.v6.l)))) {
		if (tree->left) {
			return pfx_tree_subinsert(tree->left, newnet, newmask);
		} else {
			pfx_node_t *newnode;
			newnode = (pfx_node_t*) malloc(sizeof(pfx_node_t));
			if (!newnode) {
				fprintf(stderr, "pfx_tree_subinsert: Malloc new node failure.\n");
				exit(2);
			}
			newnode->flags       = 0;
			newnode->mask        = tree->mask+1;
			newnode->left        = 0;
			newnode->right       = 0;
			tree->left           = newnode;
			if (newnet.addrtype == v4) {
				newnode->ip.addrtype = v4;
				newnode->ip.addr.v4  = tree->ip.addr.v4;
				newnode->mi.addrtype = v4;
				newnode->mi.addr.v4  = newnode->ip.addr.v4 + pfx_v4middleadds[newnode->mask].addr.v4;
				newnode->am.addrtype = v4;
				newnode->am.addr.v4  = 0;
			} else { // it's v6
				newnode->ip.addrtype  = v6;
				newnode->ip.addr.v6.h = tree->ip.addr.v6.h;
				newnode->ip.addr.v6.l = tree->ip.addr.v6.l;
				newnode->mi.addrtype  = v6;
				add128(newnode->ip, pfx_v6middleadds[newnode->mask], &newnode->mi);
				//newnode->mi.addr.v6.h = newnode->ip.addr.v6.h + v6middleadds[newnode->mask].addr.v6.h;
				//newnode->mi.addr.v6.l = newnode->ip.addr.v6.l + v6middleadds[newnode->mask].addr.v6.l;
				newnode->am.addrtype  = v6;
				newnode->am.addr.v6.h = 0;
				newnode->am.addr.v6.l = 0;
			}
			return pfx_tree_subinsert(newnode, newnet, newmask);
		}
	} else {
		// Right side insert|expand
		if (tree->right) {
			return pfx_tree_subinsert(tree->right, newnet, newmask);
		} else {
			pfx_node_t *newnode;
			newnode = (pfx_node_t*) malloc(sizeof(pfx_node_t));
			if (!newnode) {
				fprintf(stderr, "pfx_tree_subinsert: Malloc new node failure.\n");
				exit(2);
			}
			newnode->flags       = 0;
			newnode->mask        = tree->mask+1;
			newnode->left        = 0;
			newnode->right       = 0;
			tree->right          = newnode;
			if (newnet.addrtype == v4) {
				newnode->ip.addrtype = v4;
				newnode->ip.addr.v4  = tree->mi.addr.v4;
				newnode->mi.addrtype = v4;
				newnode->mi.addr.v4  = newnode->ip.addr.v4 + pfx_v4middleadds[newnode->mask].addr.v4;
				newnode->am.addrtype = v4;
				newnode->am.addr.v4  = 0;
			} else { // it's v6
				newnode->ip.addrtype  = v6;
				newnode->ip.addr.v6.h = tree->mi.addr.v6.h;
				newnode->ip.addr.v6.l = tree->mi.addr.v6.l;
				newnode->mi.addrtype  = v6;
				add128(newnode->ip, pfx_v6middleadds[newnode->mask], &newnode->mi);
				//newnode->mi.addr.v6.h = newnode->ip.addr.v6.h + v6middleadds[newnode->mask].addr.v6.h;
				//newnode->mi.addr.v6.l = newnode->ip.addr.v6.l + v6middleadds[newnode->mask].addr.v6.l;
				newnode->am.addrtype  = v6;
				newnode->am.addr.v6.h = 0;
				newnode->am.addr.v6.l = 0;
			}
			return pfx_tree_subinsert(newnode, newnet, newmask);
		}
	}
}

int pfx_tree_insert(pfx_tree_t *tree, const pfx_ipaddr_t newnet, const unsigned char newmask) {
	int retval;
	if (!tree) {
		return 0;
	}
	if (newnet.addrtype==v4) {
		retval = pfx_tree_subinsert(tree->v4, newnet, newmask);
		tree->v4size += retval;
	} else {
		retval = pfx_tree_subinsert(tree->v6, newnet, newmask);
		tree->v6size += retval;
	}
	return retval;
}

static void pfx_tree_cleanflags(pfx_node_t *tree) {
	if (tree) {
		tree->flags = tree->flags&PFXNODE_USED; // clean all other than used flag
		pfx_tree_cleanflags(tree->left);
		pfx_tree_cleanflags(tree->right);
	}
}

static void pfx_tree_subiter(pfx_node_t *tree, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, void *calldata), char aggr_abort, void *itercalldata) {
	if (!tree) {
		return;
	}
	if (tree->flags&PFXNODE_USED) {
		iterfunc(tree->ip, tree->mask, itercalldata);
		if (aggr_abort) {
			return;
		}
	} else if (aggr_abort && (tree->flags&PFXNODE_AGGR1)) {
		iterfunc(tree->ip, tree->mask, itercalldata);
		return;
	}
	pfx_tree_subiter(tree->left,  iterfunc, aggr_abort, itercalldata);
	pfx_tree_subiter(tree->right, iterfunc, aggr_abort, itercalldata);
}


void pfx_tree_iter(pfx_tree_t *tree, pfx_addr_t addrtype, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, void *calldata), void *itercalldata) {
	if (tree) {
		pfx_tree_subiter((addrtype==v4?tree->v4:tree->v6), iterfunc, 0, itercalldata);
	}
}

void pfx_tree_iteraggr0(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, void *calldata), void *calldata) {
	if (tree) {
		pfx_tree_cleanflags((v4v6==v4?tree->v4:tree->v6));
		/* just don't do this
			pfx_tree_aggr1((v4v6==v4?tree->v4:tree->v6));
		*/
		pfx_tree_subiter((v4v6==v4?tree->v4:tree->v6), iterfunc, 1, calldata);
	}
}


// see what could be aggregated; don't look deeper than needed
static void pfx_tree_aggr1(pfx_node_t *tree) {
	if (!tree || (tree->flags&PFXNODE_USED)) {
		return;
	}
	pfx_tree_aggr1(tree->left);
	pfx_tree_aggr1(tree->right);
	if (tree->left && tree->right && (tree->left->flags&(PFXNODE_USED|PFXNODE_AGGR1)) && (tree->right->flags&(PFXNODE_USED|PFXNODE_AGGR1))) {
		tree->flags = tree->flags|PFXNODE_AGGR1;
	}
}

void pfx_tree_iteraggr1(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, void *calldata), void *calldata) {
	if (tree) {
		pfx_tree_cleanflags((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_aggr1((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_subiter((v4v6==v4?tree->v4:tree->v6), iterfunc, 1, calldata);
	}
}

static void pfx_tree_aggr23(pfx_node_t *tree) {
	if (tree) {
		pfx_tree_aggr23(tree->left);
		pfx_tree_aggr23(tree->right);
		if (tree->ip.addrtype==v4) {
			tree->am.addr.v4   = ((tree->flags&PFXNODE_USED)?pfx_v4bits[tree->mask].addr.v4:0)   | ((tree->left?tree->left->am.addr.v4:0)  &(tree->right?tree->right->am.addr.v4:0));
		} else { //v6
			tree->am.addr.v6.h = ((tree->flags&PFXNODE_USED)?pfx_v6bits[tree->mask].addr.v6.h:0) | ((tree->left?tree->left->am.addr.v6.h:0)&(tree->right?tree->right->am.addr.v6.h:0));
			tree->am.addr.v6.l = ((tree->flags&PFXNODE_USED)?pfx_v6bits[tree->mask].addr.v6.l:0) | ((tree->left?tree->left->am.addr.v6.l:0)&(tree->right?tree->right->am.addr.v6.l:0));
		}
	}
}

#ifdef DEBUG
static void printbitmask(pfx_ipaddr_t mask) {
	if (mask.addrtype==v4) {
		int i;
		for (i=1; i<=32; i++) {
			fprintf(stderr, "%d", (mask.addr.v4&pfx_v4bits[i].addr.v4)?1:0);
			if (i%8==0) fprintf(stderr, ".");
		}
	} else {
		int i;
		for (i=1; i<=64; i++) {
			fprintf(stderr, "%d", (mask.addr.v6.h&pfx_v6bits[i].addr.v6.h)?1:0);
			if (i%8==0) fprintf(stderr, ".");
		}
		for (i=1; i<=64; i++) {
			fprintf(stderr, "%d", (mask.addr.v6.l&pfx_v6bits[i].addr.v6.l)?1:0);
			if (i%8==0) fprintf(stderr, ".");
		}
	}
}
#endif

static void pfx_tree_aggr4(pfx_node_t *tree) {
	if (tree) {
		pfx_tree_aggr4(tree->left);
		pfx_tree_aggr4(tree->right);
		// if one of our childs is a PFXNODE_USED, do not propagate more specifics in the bitmask, other than the ones,
		// which are directly (continous) in the sequence (J route-filter more-specific match hack)
		if (tree->ip.addrtype==v4) {
			pfx_ipaddr_t tmpaddr;
			tmpaddr.addrtype = v4;
			tmpaddr.addr.v4  = ((tree->left?tree->left->am.addr.v4:0) & (tree->right?tree->right->am.addr.v4:0));
#ifdef DEBUG
{ char str[180]; pfx_addr2str(tree->ip, str, 180);
fprintf(stderr, "pfx_tree_aggr4: ENTER NODE %s, mask=%d, flags=%s %s, tmpaddrmask=", str, tree->mask,
	(tree->flags&PFXNODE_USED)?"USED":"", 
	(tree->flags&PFXNODE_AGGR3)?"AGGR3":"");
printbitmask(tmpaddr);
fprintf(stderr, "\n");
}
#endif
			tree->am.addr.v4 = ((tree->flags&PFXNODE_USED)?pfx_v4bits[tree->mask].addr.v4:0);
			if ((tree->left && (tree->left->flags&PFXNODE_USED)) || (tree->right && (tree->right->flags&PFXNODE_USED))) {
				// copy continues bits only to my mask 
				int i;
				for (i=tree->mask+1; i<=32 && (tmpaddr.addr.v4 & pfx_v4bits[i].addr.v4); i++) {
					tree->am.addr.v4 |= pfx_v4bits[i].addr.v4;
					//fprintf(stderr, "%d:%s\n", i, (tmpaddr.addr.v4 & pfx_v4bits[i].addr.v4)?"1":0);
				}
				// hit first 0 or end, so we are done
			} else {
				//fprintf(stderr, "left or right is not used or not available\n");
				tree->am.addr.v4 |= ((tree->left?tree->left->am.addr.v4:0) & (tree->right?tree->right->am.addr.v4:0));
			}
#ifdef DEBUG
{ char str[180]; pfx_addr2str(tree->ip, str, 180);
	fprintf(stderr, "pfx_tree_aggr4: LEAVING NODE %s, mask=%d, flags=%s %s, am=", str, tree->mask,
	(tree->flags&PFXNODE_USED)?"USED":"", 
	(tree->flags&PFXNODE_AGGR3)?"AGGR3":"");
printbitmask(tree->am);
fprintf(stderr, "\n");
}
#endif

			
		} else { //v6
			pfx_ipaddr_t tmpaddr;
			tmpaddr.addrtype = v6;
			tmpaddr.addr.v6.h= ((tree->left?tree->left->am.addr.v6.h:0)&(tree->right?tree->right->am.addr.v6.h:0));
			tmpaddr.addr.v6.l= ((tree->left?tree->left->am.addr.v6.l:0)&(tree->right?tree->right->am.addr.v6.l:0));
			tree->am.addr.v6.h = ((tree->flags&PFXNODE_USED)?pfx_v6bits[tree->mask].addr.v6.h:0);
			tree->am.addr.v6.l = ((tree->flags&PFXNODE_USED)?pfx_v6bits[tree->mask].addr.v6.l:0);
			if ((tree->left && (tree->left->flags&PFXNODE_USED)) || (tree->right && (tree->right->flags&PFXNODE_USED))) {
				int i;
				for (i=tree->mask+1; i<=128 && ((tmpaddr.addr.v6.h & pfx_v6bits[i].addr.v6.h)||(tmpaddr.addr.v6.l & pfx_v6bits[i].addr.v6.l)); i++) {
					tree->am.addr.v6.h |= pfx_v6bits[i].addr.v6.h;
					tree->am.addr.v6.l |= pfx_v6bits[i].addr.v6.l;
				}
				// hit first 0 or the end
			} else {
				tree->am.addr.v6.h |= ((tree->left?tree->left->am.addr.v6.h:0)&(tree->right?tree->right->am.addr.v6.h:0));
				tree->am.addr.v6.l |= ((tree->left?tree->left->am.addr.v6.l:0)&(tree->right?tree->right->am.addr.v6.l:0));
			}
		}
	}
}


static void pfx_tree_subaggr23set(pfx_node_t *tree, pfx_ipaddr_t cm, int flagmask) {
	if (tree) {
		if (cm.addrtype==v4) {
			if ( cm.addr.v4&pfx_v4bits[tree->mask].addr.v4 ) {
				tree->flags |= flagmask;
				cm.addr.v4 &= ~pfx_v4bits[tree->mask].addr.v4;
			}
			if (cm.addr.v4) {
				pfx_tree_subaggr23set(tree->left, cm, flagmask);
				pfx_tree_subaggr23set(tree->right, cm, flagmask);
			}
		} else {
			if ( (cm.addr.v6.h&pfx_v6bits[tree->mask].addr.v6.h) || (cm.addr.v6.l&pfx_v6bits[tree->mask].addr.v6.l) ) {
				tree->flags |= flagmask;
				cm.addr.v6.h &= ~pfx_v6bits[tree->mask].addr.v6.h;
				cm.addr.v6.l &= ~pfx_v6bits[tree->mask].addr.v6.l;
			}
			if (cm.addr.v6.h || cm.addr.v6.l) {
				pfx_tree_subaggr23set(tree->left, cm, flagmask);
				pfx_tree_subaggr23set(tree->right, cm, flagmask);
			}
		}
	}
}

static void pfx_tree_subaggr3set(pfx_node_t *tree, pfx_ipaddr_t cm, int flagmask) {
	if (tree) {
		if (cm.addrtype==v4) {
			if ( cm.addr.v4&pfx_v4bits[tree->mask].addr.v4 ) {
				tree->flags |= flagmask;
				cm.addr.v4 &= ~pfx_v4bits[tree->mask].addr.v4;
			}
			// clean the way down all subnodes settings for pfx already picked up, if it's not used
			//if (!(tree->flags&PFXNODE_USED)) {
				tree->am.addr.v4 &= ~cm.addr.v4;
		//}
			if (cm.addr.v4) {
				pfx_tree_subaggr3set(tree->left, cm, flagmask);
				pfx_tree_subaggr3set(tree->right, cm, flagmask);
			}
		} else {
			if ( (cm.addr.v6.h&pfx_v6bits[tree->mask].addr.v6.h) || (cm.addr.v6.l&pfx_v6bits[tree->mask].addr.v6.l) ) {
				tree->flags |= flagmask;
				cm.addr.v6.h &= ~pfx_v6bits[tree->mask].addr.v6.h;
				cm.addr.v6.l &= ~pfx_v6bits[tree->mask].addr.v6.l;
			}
			//if (!(tree->flags&PFXNODE_USED)) {
				tree->am.addr.v6.h &= ~cm.addr.v6.h;
				tree->am.addr.v6.l &= ~cm.addr.v6.l;
			//}
			if (cm.addr.v6.h || cm.addr.v6.l) {
				pfx_tree_subaggr3set(tree->left, cm, flagmask);
				pfx_tree_subaggr3set(tree->right, cm, flagmask);
			}
		}
	}
}

 
static void pfx_tree_subiteraggr2(pfx_node_t *tree, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, unsigned char upto, void *calldata), void *iterdata) {
	if (tree) {
		if (tree->flags&PFXNODE_USED && !(tree->flags&PFXNODE_AGGR2)) { // we might be the beginning of an upto line
			int i;
			pfx_ipaddr_t cm;
			if (tree->ip.addrtype==v4) {
				cm.addrtype = v4;
				cm.addr.v4 = 0;
				for (i=tree->mask; i<32 && (tree->am.addr.v4&pfx_v4bits[i+1].addr.v4); i++) {
					cm.addr.v4 |= pfx_v4bits[i+1].addr.v4;
				}
				// clean our own flag
				// cm.addr.v4 &= ~pfx_v4bits[tree->mask].addr.v4;
			} else {
				cm.addrtype = v6;
				cm.addr.v6.h = 0;
				cm.addr.v6.l = 0;
				for (i=tree->mask; i<128 && ((tree->am.addr.v6.h&pfx_v6bits[i+1].addr.v6.h) || (tree->am.addr.v6.l&pfx_v6bits[i+1].addr.v6.l) ); i++) {
					cm.addr.v6.h |= pfx_v6bits[i+1].addr.v6.h;
					cm.addr.v6.l |= pfx_v6bits[i+1].addr.v6.l;
				}
				// cm.addr.v6.h &= ~pfx_v6bits[tree->mask].addr.v6.l;
				// cm.addr.v6.l &= ~pfx_v6bits[tree->mask].addr.v6.l;
			}
			iterfunc(tree->ip, tree->mask, i, iterdata);
			
			pfx_tree_subaggr23set(tree->left, cm, PFXNODE_AGGR2);
			pfx_tree_subaggr23set(tree->right, cm, PFXNODE_AGGR2);
			
		}
		pfx_tree_subiteraggr2(tree->left, iterfunc, iterdata);
		pfx_tree_subiteraggr2(tree->right, iterfunc, iterdata);
	}
}

void pfx_tree_iteraggr2(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, unsigned char upto, void *calldata), void *iterdata) {
	if (tree) {
		pfx_tree_cleanflags((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_aggr23((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_subiteraggr2((v4v6==v4?tree->v4:tree->v6), iterfunc, iterdata);
	}
}


 
 static void pfx_tree_subiteraggr3(pfx_node_t *tree, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, unsigned char from, unsigned char to, void *calldata), void *iterdata) {
	if (tree) {
//		if (tree->flags&PFXNODE_USED && !(tree->flags&PFXNODE_AGGR3)) { // we might be the beginning of an upto line
#ifdef DEBUG
{ char str[180]; pfx_addr2str(tree->ip, str, 180);
fprintf(stderr, "pfx_tree_subiteraggr3: NODE %s, mask=%d, flags=%s %s, am=", str, tree->mask,
	(tree->flags&PFXNODE_USED)?"USED":"", 
	(tree->flags&PFXNODE_AGGR3)?"AGGR3":"");
printbitmask(tree->am);
fprintf(stderr, "\n");
}
#endif
		if (!(tree->flags&PFXNODE_AGGR3)) { 
			// this node was not yet reported before
			int i;
			pfx_ipaddr_t cm;
			
			if (tree->ip.addrtype==v4) {
			cm.addrtype = v4;
			cm.addr.v4 = 0;
			
			if (tree->am.addr.v4) {
			
				cm.addrtype = v4;
				cm.addr.v4 = pfx_v4bits[tree->mask].addr.v4;
				//cm.addr.v4 = 0;
#ifdef DEBUG				
fprintf(stderr, "START\n am="); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
				i = tree->mask;
				if (tree->flags&PFXNODE_USED) {
#ifdef DEBUG
fprintf(stderr, "NODE IS used - building upto info ...\n");
#endif
					// first the "upto" line

					for (i=tree->mask; i<32 && (tree->am.addr.v4&pfx_v4bits[i+1].addr.v4); i++) {
						cm.addr.v4 |= pfx_v4bits[i+1].addr.v4;
#ifdef DEBUG
fprintf(stderr, "i=%d am=", i); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
					}
#ifdef DEBUG
fprintf(stderr, "Call UPTO with %d-%d\n", tree->mask, i);
#endif
					iterfunc(tree->ip, tree->mask, i, 255, iterdata);
				}
				// Then find other "continues" (1 or more) bit sequences - but only "BEHIND" our mask
#ifdef DEBUG
fprintf(stderr, "BEFORE WHILE\n am="); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
				i++;
				while ( tree->am.addr.v4 & ~cm.addr.v4 ) { // we still have not all in our cleanmask
					int j;
					// skip zeros
#ifdef DEBUG
fprintf(stderr, "OUTERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v4bitmasks[mask]="); printbitmask(pfx_v4bits[i]); fprintf(stderr, "\n");
#endif
					while ( i<=32 && !(tree->am.addr.v4&pfx_v4bits[i].addr.v4) ) {
#ifdef DEBUG
fprintf(stderr, "INNERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v4bitmasks[i]="); printbitmask(pfx_v4bits[i]); fprintf(stderr, "\n");
#endif
						i++;
					} 
					if (i<=32) { 
#ifdef DEBUG
	fprintf(stderr, "AFTERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v4bitmasks[i]="); printbitmask(pfx_v4bits[i]); fprintf(stderr, "\n");
#endif
						// now find the current "length"
						cm.addr.v4 |= pfx_v4bits[i].addr.v4;
#ifdef DEBUG
	fprintf(stderr, "BEFOREFOR mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v4bitmasks[i]="); printbitmask(pfx_v4bits[i]); fprintf(stderr, "\n");
#endif
						for (j=i; j<32 && (tree->am.addr.v4&pfx_v4bits[j+1].addr.v4); j++) {
							cm.addr.v4 |= pfx_v4bits[j+1].addr.v4;
#ifdef DEBUG
	fprintf(stderr, "INNERFOR mask=%d, j=%d, cm=", tree->mask, j); printbitmask(cm); fprintf(stderr, " v4bitmasks[j+1=%d]=", j+1); printbitmask(pfx_v4bits[j+1]); fprintf(stderr, "\n");
#endif
						}
#ifdef DEBUG
	fprintf(stderr, "AFTERFOR mask=%d, i=%d, j=%d, cm=", tree->mask, i, j); printbitmask(cm); fprintf(stderr, " v4bitmasks[j+1=%d]=", j+1); printbitmask(pfx_v4bits[j+1]); fprintf(stderr, "\n");
	fprintf(stderr, "CALL SUBRANGE /%d,%d-%d\n", tree->mask, i,j);
#endif
						iterfunc(tree->ip, tree->mask, i, j, iterdata);
					} else {
#ifdef DEBUG
	fprintf(stderr, "NO MORE SET BITs found\n");
#endif
					}
	//fprintf(stderr, "tm
					i=j+1;
				}
				// clean our own flag for subtree marking
				cm.addr.v4 &= ~pfx_v4bits[tree->mask].addr.v4;
				
				
			}

			} else { // v6


				cm.addrtype = v6;
				cm.addr.v6.h = pfx_v6bits[tree->mask].addr.v6.h;
				cm.addr.v6.l = pfx_v6bits[tree->mask].addr.v6.l;
				//cm.addr.v4 = 0;
#ifdef DEBUG				
fprintf(stderr, "START6\n am="); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
				i = tree->mask;
				if (tree->flags&PFXNODE_USED) {
#ifdef DEBUG
fprintf(stderr, "NODE6 IS used - building upto info ...\n");
#endif
					// first the "upto" line

					for (i=tree->mask; i<128 && ((tree->am.addr.v6.h&pfx_v6bits[i+1].addr.v6.h)||(tree->am.addr.v6.l&pfx_v6bits[i+1].addr.v6.l)); i++) {
						cm.addr.v6.h |= pfx_v6bits[i+1].addr.v6.h;
						cm.addr.v6.l |= pfx_v6bits[i+1].addr.v6.l;
#ifdef DEBUG
fprintf(stderr, "6i=%d am=", i); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
					}
#ifdef DEBUG
fprintf(stderr, "Call UPTO6 with %d-%d\n", tree->mask, i);
#endif
					iterfunc(tree->ip, tree->mask, i, 255, iterdata);
				}
				// Then find other "continues" (1 or more) bit sequences - but only "BEHIND" our mask
#ifdef DEBUG
fprintf(stderr, "6BEFORE WHILE\n am="); printbitmask(tree->am); fprintf(stderr, " cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
				i++;
				while ( (tree->am.addr.v6.h & ~cm.addr.v6.h) || (tree->am.addr.v6.l & ~cm.addr.v6.l)) { // we still have not all in our cleanmask
					int j;
					// skip zeros
#ifdef DEBUG
fprintf(stderr, "6OUTERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v6bitmasks[mask]="); printbitmask(pfx_v6bits[i]); fprintf(stderr, "\n");
#endif
					while ( i<=128 && !((tree->am.addr.v6.h&pfx_v6bits[i].addr.v6.h)||(tree->am.addr.v6.l&pfx_v6bits[i].addr.v6.l)) ) {
#ifdef DEBUG
fprintf(stderr, "6INNERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v6bitmasks[i]="); printbitmask(pfx_v6bits[i]); fprintf(stderr, "\n");
#endif
						i++;
					} 
					if (i<=128) { 
#ifdef DEBUG
	fprintf(stderr, "6AFTERWHILE mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v6bitmasks[i]="); printbitmask(pfx_v6bits[i]); fprintf(stderr, "\n");
#endif
						// now find the current "length"
						cm.addr.v6.h |= pfx_v6bits[i].addr.v6.h;
						cm.addr.v6.l |= pfx_v6bits[i].addr.v6.l;
						
#ifdef DEBUG
	fprintf(stderr, "6BEFOREFOR mask=%d, i=%d, cm=", tree->mask, i); printbitmask(cm); fprintf(stderr, " v6bitmasks[i]="); printbitmask(pfx_v6bits[i]); fprintf(stderr, "\n");
#endif
						for (j=i; j<128 && ((tree->am.addr.v6.h&pfx_v6bits[j+1].addr.v6.h)||(tree->am.addr.v6.l&pfx_v6bits[j+1].addr.v6.l)); j++) {
							cm.addr.v6.h |= pfx_v6bits[j+1].addr.v6.h;
							cm.addr.v6.l |= pfx_v6bits[j+1].addr.v6.l;
#ifdef DEBUG
	fprintf(stderr, "6INNERFOR mask=%d, j=%d, cm=", tree->mask, j); printbitmask(cm); fprintf(stderr, " v6bitmasks[j+1=%d]=", j+1); printbitmask(pfx_v6bits[j+1]); fprintf(stderr, "\n");
#endif
						}
#ifdef DEBUG
	fprintf(stderr, "6AFTERFOR mask=%d, i=%d, j=%d, cm=", tree->mask, i, j); printbitmask(cm); fprintf(stderr, " v6bitmasks[j+1=%d]=", j+1); printbitmask(pfx_v6bits[j+1]); fprintf(stderr, "\n");
	fprintf(stderr, "6CALL SUBRANGE /%d,%d-%d\n", tree->mask, i,j);
#endif
						iterfunc(tree->ip, tree->mask, i, j, iterdata);
					} else {
#ifdef DEBUG
	fprintf(stderr, "NO MORE SET BITs found\n");
#endif
					}
	//fprintf(stderr, "tm
					i=j+1;
				}
				// clean our own flag for subtree marking
				cm.addr.v6.h &= ~pfx_v6bits[tree->mask].addr.v6.h;
				cm.addr.v6.l &= ~pfx_v6bits[tree->mask].addr.v6.l;


			}

#ifdef DEBUG
	fprintf(stderr, "CLEAN LEFT: cm="); printbitmask(cm); fprintf(stderr, "\n");
#endif
			pfx_tree_subaggr3set(tree->left, cm, PFXNODE_AGGR3);
#ifdef DEBUG
fprintf(stderr, "CLEAN LEFT: RIGHT="); printbitmask(cm); fprintf(stderr, "\n");
#endif
			pfx_tree_subaggr3set(tree->right, cm, PFXNODE_AGGR3);
			
		}
		pfx_tree_subiteraggr3(tree->left, iterfunc, iterdata);
		pfx_tree_subiteraggr3(tree->right, iterfunc, iterdata);
	}
}


void pfx_tree_iteraggr3(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, unsigned char from, unsigned char to, void *calldata), void *iterdata) {
	if (tree) {
		pfx_tree_cleanflags((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_aggr23((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_subiteraggr3((v4v6==v4?tree->v4:tree->v6), iterfunc, iterdata);
	}
}

void pfx_tree_iteraggr4(pfx_tree_t *tree, pfx_addr_t v4v6, void (*iterfunc)(pfx_ipaddr_t net, unsigned char mask, unsigned char from, unsigned char to, void *calldata), void *iterdata) {
	if (tree) {
		pfx_tree_cleanflags((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_aggr4((v4v6==v4?tree->v4:tree->v6));
		pfx_tree_subiteraggr3((v4v6==v4?tree->v4:tree->v6), iterfunc, iterdata);
	}
}

