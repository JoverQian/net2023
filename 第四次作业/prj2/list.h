#ifndef __LIST_H__
#define __LIST_H__

#include <stdlib.h>
#include <stdint.h>

//Linked list information
enum LLtype 
{
    llt_string,
    llt_frame,
    llt_integer,
    llt_head
} LLtype;

struct LLnode_t
{
    struct LLnode_t * prev;
    struct LLnode_t * next;
    enum LLtype type;

    void * value;
};
typedef struct LLnode_t LLnode;

//Linked list functions
int ll_get_length(LLnode *);
void ll_append_node(LLnode **, void *);
LLnode * ll_pop_node(LLnode **);
void ll_destroy_node(LLnode *);


#endif