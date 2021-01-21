#ifndef MEM_QUEUE_H
#define MEM_QUEUE_H

typedef struct mem_queue {
	
	void *data;
	struct mem_queue *prev;
	struct mem_queue *next;
		
}mem_queue_t; 

mem_queue_t *mem_queue_init(void);
mem_queue_t *mem_queue_put_last(mem_queue_t *root,mem_queue_t *node);
mem_queue_t *mem_queue_put_first(mem_queue_t *root,mem_queue_t *node);
void *mem_queue_get_last(mem_queue_t *root);
mem_queue_t *mem_queue_get_first(mem_queue_t *root);

#endif
