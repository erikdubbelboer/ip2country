
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../db.h"


#ifndef max
	#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif


static range_tree_t* root = NULL;
static unsigned int  next_id = 0;

static country_list_t* countries   = NULL;
static unsigned int    countries_s = 0;



static country_list_t* country_add(char* code, char* name) {
	if (countries == NULL) {
		countries = (country_list_t*)malloc(sizeof(country_list_t));

		strcpy(countries->data.code, code);
		strcpy(countries->data.name, name);

		countries->next = NULL;
		countries->id   = countries_s++;

		return countries;
	}


	country_list_t* country = countries;

	for (;;) {
		if (strcmp(country->data.code, code) == 0) {
			return country;
		}

		if (country->next == NULL) {
			break;
		} else {
			country = country->next;
		}
	}

	country->next = (country_list_t*)malloc(sizeof(country_list_t));
	country = country->next;

	strcpy(country->data.code, code);
	strcpy(country->data.name, name);

	country->next = NULL;
	country->id   = countries_s++;

	return country;
}



static range_tree_t* tree_left(unsigned long* flags, range_tree_t* node) {
	if (((*flags) & done_left) ||
		(node->left == NULL      )) {
		return NULL;
	} else {
		(*flags) = 0;

		return node->left;
	}
}


static range_tree_t* tree_right(unsigned long* flags, range_tree_t* node) {
	if (((*flags) & done_right) ||
		(node->right == NULL      )) {
		return NULL;
	} else {
		(*flags) = 0;

		return node->right;
	}
}


static range_tree_t* tree_up(unsigned long* flags, range_tree_t* node) {
	if (node->parent == NULL) {
		return NULL;
	} else {
		if (node->parent->left == node) {
			(*flags) = done_left | done_this;
		} else {
			(*flags) = done_left | done_this | done_right;
		}

		return node->parent;
	}
}


static range_tree_t* tree_deepest(range_tree_t* node) {
	unsigned int ldepth = (node->left  == NULL) ? 0 : node->left->depth;
	unsigned int rdepth = (node->right == NULL) ? 0 : node->right->depth;

	if (ldepth > rdepth) {
		return node->left;
	} else {
		return node->right;
	}
}


/* return the root of a new balanced tree */
static range_tree_t* tree_rotate(range_tree_t* a) {
	/* no need to check anything because the tree is unbalanced so none of these will be NULL */
	range_tree_t* b = tree_deepest(a);
	range_tree_t* c = tree_deepest(b);

	range_tree_t* d1, * d2, * d3, * d4;
	range_tree_t *r;

	/* there are 4 different unbalanced trees */
	if (a->left == b) {
		if (b->left == c) {
			//       a              b     
			//      / \           /   \    
			//     b   4         c     a   
			//    / \           / \   / \  
			//   c   3         1   2 3   4
			//  / \
			// 1   2
			d1 = c->left;
			d2 = c->right;
			d3 = b->right;
			d4 = a->right;

			r        = b;
			r->left  = c;
			r->right = a;
		} else {
			//        a             c     
			//       / \          /   \    
			//     b    4        b     a   
			//    / \           / \   / \  
			//   1   c         1   2 3   4
			//      / \
			//     2   3
			d1 = b->left;
			d2 = c->left;
			d3 = c->right;
			d4 = a->right;

			r        = c;
			r->left  = b;
			r->right = a;
		}
	} else {
		if (b->left == c) {
			//       a              c     
			//      / \           /   \    
			//     1    b        a     b   
			//         / \      / \   / \  
			//        c   4    1   2 3   4
			//       / \
			//      2   3
			d1 = a->left;
			d2 = c->left;
			d3 = c->right;
			d4 = b->right;

			r        = c;
			r->left  = a;
			r->right = b;
		} else {
			//        a             b     
			//       / \          /   \    
			//      1   b        a     c   
			//         / \      / \   / \  
			//        2   c    1   2 3   4
			//           / \
			//          3   4
			d1 = a->left;
			d2 = b->left;
			d3 = c->left;
			d4 = c->right;

			r        = b;
			r->left  = a;
			r->right = c;
		}
	}

	r->left->parent  = r;
	r->right->parent = r;

	r->left->left   = d1;
	r->left->right  = d2;
	r->right->left  = d3;
	r->right->right = d4;

	if (d1 != NULL) d1->parent = r->left;
	if (d2 != NULL) d2->parent = r->left;
	if (d3 != NULL) d3->parent = r->right;
	if (d4 != NULL) d4->parent = r->right;

	r->left->depth  = max((d1 == NULL) ? 1 : d1->depth + 1,
			      (d2 == NULL) ? 1 : d2->depth + 1);
	r->right->depth = max((d3 == NULL) ? 1 : d3->depth + 1,
			      (d4 == NULL) ? 1 : d4->depth + 1);

	r->depth = max(r->left->depth, r->right->depth) + 1;
	
	return r;
}



int tree_insert(range_tree_t* node) {
	if (root == NULL) { // no root
		node->parent        = NULL;
		node->left          = node->right = NULL;
		node->depth         = 1;
		root		    = node;
	} else {
		int           depth;
		range_tree_t* curr = root;

		/* walk down the tree to search for a place to insert the node */
		for (;;) {
			if (node->start == curr->start) {
				return -1;
			}

			if (node->start < curr->start) {
				if (curr->left == NULL) {
					curr->left         = node;
					curr->left->parent = curr;
					break;
				} else {
					curr = curr->left;
				}
			} else {
				if (curr->right == NULL) {
					curr->right         = node;
					curr->right->parent = curr;
					break;
				} else {
					curr = curr->right;
				}
			}
		}

		node->left  = node->right = NULL;
		node->depth = 1;

		for (node = node->parent, depth = 2; node != NULL; node = node->parent, depth++) {
			int ldepth, rdepth;

			// if the depth of this node doesn't change,
			// the depths of the nodes above won't change either.
			if (node->depth >= depth) {
				break;
			}

			ldepth = (node->left  == NULL) ? 0 : node->left->depth;
			rdepth = (node->right == NULL) ? 0 : node->right->depth;

			if (abs(ldepth - rdepth) > 1) {
				range_tree_t* p;

				if (node->parent == NULL) { // root
					root         = tree_rotate(node);
					root->parent = NULL;
				} else if (node->parent->left == node) {
					p               = node->parent;
					p->left         = tree_rotate(node);
					p->left->parent = p;
				} else {
					p                = node->parent;
					p->right         = tree_rotate(node);
					p->right->parent = p;
				}
				break;
			} else {
				node->depth = depth;
			}
		}
	}

	return 0;
}



// write all ranges who's depth equals de depth given as argument
void write_with_depth(unsigned int depth, FILE* fp) {
	range_tree_t* node = root;
	unsigned long flags = 0;
	range_t       node_tmp;

	unsigned int cdepth = 0;

	for (;;) {
		range_tree_t* curr = node;

		if (!(flags & done_this) && (cdepth == depth)) {
			memset(&node_tmp, 0, sizeof(range_t));

			node_tmp.left    = (curr->left  != NULL) ? curr->left->id  : UINT_MAX;
			node_tmp.right   = (curr->right != NULL) ? curr->right->id : UINT_MAX;
			node_tmp.start   = curr->start;
			node_tmp.end     = curr->end;
			node_tmp.country = curr->country->id;

			fwrite(&node_tmp, sizeof(range_t), 1, fp);

			curr->id = next_id++;
		}

		node = tree_left(&flags, curr);

		if (node == NULL) {
			node = tree_right(&flags, curr);

			if (node == NULL) {
				node = tree_up(&flags, curr);

				if (node == NULL) {
					return;
				} else {
					cdepth--;
				}
			} else {
				cdepth++;
			}
		} else {
			cdepth++;
		}
	}
}




int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("usage: makedb <input file> <output file>\n");
		return 0;
	}

	
	FILE* fp = fopen(argv[1], "r");

	if (fp == NULL) {
		printf("error: could not open '%s' for reading\n", argv[1]);
		return -1;
	}


	unsigned int  start = 0;
	unsigned int  end   = 0;
	char          code[4];
	char	      name[60];
	char	      dummy[64];
	range_tree_t* node;
	
	memset(code, 0, sizeof(code));
	memset(name, 0, sizeof(name));

	unsigned int rows = 0;

	while (fscanf(fp, "%[^,],%[^,],\"%u\",\"%u\",\"%2c\",\"%59[^\"]\"", &dummy, &dummy, &start, &end, code, name) == 6) {
		node          = (range_tree_t*)malloc(sizeof(range_tree_t));
		node->start   = start;
		node->end     = end;
		node->country = country_add(code, name);

		if (tree_insert(node) == -1) {
			return -1;
		}

		rows++;
	}

	fclose(fp);

	printf("%u ranges read from the input file\n", rows);

	if (rows == 0) {
		printf("nothing to output.\n");
		return 0;
	}



	fp = fopen(argv[2], "wb");

	if (fp == NULL) {
		printf("error: could not open '%s' for writing\n", argv[2]);
		return -2;
	}



	fwrite(&countries_s, sizeof(unsigned int), 1, fp);

	country_list_t* country = countries;

	while (country != NULL) {
		fwrite(&country->data, sizeof(country_t), 1, fp);

		country = country->next;
	}

	
	fwrite(&rows, sizeof(unsigned int), 1, fp);

	int depth;
	for (depth = root->depth; depth >= 0; --depth) {
		write_with_depth(depth, fp);
	}


	fclose(fp);

	printf("done.\n");

	return 0;
}

