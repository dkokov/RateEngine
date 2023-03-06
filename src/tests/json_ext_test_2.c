/*
 * gcc -Wall -g -o json_ext_test_2 json_ext_test_2.c -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_ext_test_2
 * 
 * This example tests 'json_ext' as uses copy by 'static json_ext_obj_t template' in the dynamic memory allocation,
 * convert 'C data types' to 'JSON format' and write to file
 * 
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

static json_ext_obj_t test2[] = {
	{json_str,"object2"},
	{0,""}
};

static json_ext_obj_t test_arr[] = {
	{json_int_arr,"rating_id"},
	{json_str_arr,"prefix"},
	{json_str_arr,"comm"},
	{0,""}
};

typedef struct test_struct {
	unsigned int id;
	unsigned short tt;
	unsigned short tt2;
	double dd;
	char ttt[256];
} test_struct_t;

int main(void)
{
	int  rt_ids[10];
	
	json_ext_obj_t *new,*new_arr,*new2;
	
	json_ext_str2_t rt_prefix[10];
	json_ext_str2_t rt_comm[10];
	
	int i;
	char *buf;
	char test_str[] = "test-1111";
	
	test_struct_t *tttt;
	
	for(i=0;i<10;i++) {
		rt_ids[i] = i+1;
		sprintf(rt_prefix[i].str,"359%d",i);
		sprintf(rt_comm[i].str,"comm %s",rt_prefix[i].str);
	}

	tttt = (test_struct_t *)calloc(1,sizeof(test_struct_t));
	strcpy(tttt->ttt,"test-test-1234");
	tttt->tt  = 2;
	tttt->tt2 = 3;
	tttt->id  = 11;
	tttt->dd  = 1.123456;
	
	new = json_ext_new_obj(test);
	new_arr = json_ext_new_obj(test_arr);
	new2 = json_ext_new_obj(test2);
	
	json_ext_init_value(new2,"object2",(void *)test_str,0);
	
	json_ext_init_value(new,"call-uid",(void *)tttt->ttt,0);
	json_ext_init_value(new,"cdr_id",(void *)&tttt->id,0);
	json_ext_init_value(new,"cdr-rec-type",(void *)&tttt->tt2,0);
	json_ext_init_value(new,"amount",(void *)&tttt->dd,0);
	json_ext_init_value(new_arr,"rating_id",(void *)rt_ids,0);
	json_ext_init_value(new_arr,"prefix",(void *)rt_prefix,0);
	json_ext_init_value(new_arr,"comm",(void *)rt_comm,0);
	json_ext_init_value(new,"rating",(void *)new_arr,10);
	json_ext_init_value(new,"file",(void *)test_str,0); 
	json_ext_init_value(new,"obj2",(void *)new2,0);

	buf = json_ext_obj_create(new);
	
	free(tttt);

	json_ext_delete_obj(new,-1);
	json_ext_delete_obj(new_arr,-1);
	json_ext_delete_obj(new2,-1);
	
	printf("\nbuf(write): %s\n\n",buf);

	json_ext_write_file("test_2.json",buf);
	
	free(buf);
	
	return 0;
}

