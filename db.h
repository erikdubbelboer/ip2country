
#ifndef _db_h_
#define _db_h_



#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */



/* flags used while traversing the tree */
#define done_left  (1<<0)
#define done_this  (1<<1)
#define done_right (1<<2)




typedef struct country_s {
	char code[4];
	char name[60];
} country_t;


/* this struct is only used to build the list */
typedef struct country_list_s {
	country_t data;

	struct country_list_s* next;
	unsigned int           id;
} country_list_t;




typedef struct range_s {
	/* numbers of the childs in the tree */
	unsigned int left;
	unsigned int right;
	
	unsigned int start;
	unsigned int end;

	unsigned short country;
} range_t;


/* this version is only used to build the tree */
typedef struct range_tree_s {
	struct range_tree_s* left;
	struct range_tree_s* right;
	
	unsigned int    start;
	unsigned int    end;
	country_list_t* country;

	int                  depth;
	struct range_tree_s* parent;
	unsigned int         id;
} range_tree_t;




#ifdef __cplusplus
 }
#endif /* __cplusplus */




#endif /* _db_h_ */

