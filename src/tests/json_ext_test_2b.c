/*
 * gcc -Wall -g -o json_ext_test_2b json_ext_test_2b.c -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_ext_test_2b
 * 
 * This example tests 'json_ext' as uses copy by 'static json_ext_obj_t template' in the dynamic memory allocation,
 * read file and convert 'JSON format' to 'C data types'
 * 
 * */
 
#include <stdio.h>
#include <string.h>

#include "../misc/exten/json_ext.h"

static json_ext_obj_t test[] = {
	{json_str,"call-uid"},
	{json_uint,"cdr_id"},
	{json_ushrt,"cdr_server_id"},
	{json_ushrt,"cdr-rec-type"},
	{json_double,"amount"},
	{json_arr,"rating"},
	{json_str,"file"},
	{json_obj,"obj2"},
	{0,""}
};

static json_ext_obj_t test_arr[] = {
	{json_int_arr,"rating_id"},
	{json_str_arr,"prefix"},
	{json_str_arr,"comm"},
	{0,""}
};

static json_ext_obj_t test2[] = {
	{json_str,"object2"},
	{0,""}
};

int main(void)
{	
	json_ext_obj_t *new,*new_arr,*new2;
	
	char *buf;
	
	json_ext_read_file("test_2.json",&buf);
		
	printf("\nbuf(read): %s\n\n",buf);

	json_object *jobj = json_tokener_parse(buf);
	
	new = json_ext_new_obj(test);
	new_arr = json_ext_new_obj(test_arr);
	json_ext_init_value(new,"rating",(void *)new_arr,0);
	new2 = json_ext_new_obj(test2);
	json_ext_init_value(new,"obj2",(void *)new2,0);
	
	json_ext_get_obj(jobj,new);
	
	json_ext_print_obj(new,0);
	
	json_ext_delete_obj(new,0);
	
	json_object_put(jobj);
	
	free(buf);

	return 0;
}

