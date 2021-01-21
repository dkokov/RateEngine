/*
 * gcc -Wall -g -o json_ext_test json_ext_test.c -ljson-c -L../ -lre7core
 * 
 * valgrind --leak-check=full --show-leak-kinds=all ./json_ext_test 
 * 
 * This example tests 'json_ext' as uses 'static json_ext_obj_t',without dynamic memory allocation
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
	{json_str,"test-test"},
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

	json_ext_init_value(test,"call-uid",(void *)tttt->ttt,0);
	json_ext_init_value(test,"cdr_id",(void *)&tttt->id,0);
	json_ext_init_value(test,"cdr-rec-type",(void *)&tttt->tt2,0);
	json_ext_init_value(test,"amount",(void *)&tttt->dd,0);
	json_ext_init_value(test_arr,"rating_id",(void *)rt_ids,0);
	json_ext_init_value(test_arr,"prefix",(void *)rt_prefix,0);
	json_ext_init_value(test_arr,"comm",(void *)rt_comm,0);
	json_ext_init_value(test,"rating",(void *)test_arr,10);
	json_ext_init_value(test,"file",(void *)test_str,0); 

	buf = json_ext_obj_create(test);
	
	printf("\nbuf: %s\n\n",buf);

	json_ext_write_file("test.json",buf);
	
	free(buf);
	free(tttt);
	
	return 0;
}

