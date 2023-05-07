#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "mem_queue.h" 

mem_queue_t *mem_queue_init(void)
{
	return mem_alloc(sizeof(mem_queue_t));
}

mem_queue_t *mem_queue_put_last(mem_queue_t *root,mem_queue_t *node)
{
	if(root == NULL) {
		/* The tree is empty */
		root = node;
	} else {
		/* The tree is already init */
		mem_queue_t *tmp = root;
		
		loop:
		if(tmp->next == NULL) {
			tmp->next = node;
			node->prev = tmp;
		} else {
			tmp = tmp->next;
			goto loop;
		}
	}
	
	return root;	
}

mem_queue_t *mem_queue_put_first(mem_queue_t *root,mem_queue_t *node)
{
	if(root == NULL) {
		/* The tree is empty */
		root = node;
	} else {
		/* The tree is already init */
		mem_queue_t *tmp = root;
		
		node->next = tmp;
		tmp->prev = node;
		root = node;
	}
	
	return root;	
}

void *mem_queue_get_last(mem_queue_t *root)
{
	void *data = NULL;

	if(root != NULL) {
		mem_queue_t *tmp = root;
		
		loop:
		if(tmp->next == NULL) {
			data = tmp->data;
			
			//tmp->next = node;
			//node->prev = tmp;
		} else {
			tmp = tmp->next;
			goto loop;
		}
	}
	
	return data;
}

mem_queue_t *mem_queue_get_first(mem_queue_t *root)
{	
	if(root != NULL) {
		mem_queue_t *tmp = root;
		
		root = tmp->next;
		root->prev= NULL;
		
		mem_free(tmp);
		
	} else return NULL;
	
	return root;
}
