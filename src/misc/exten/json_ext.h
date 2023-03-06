#ifndef JSON_EXT_H
#define JSON_EXT_H

#include <json-c/json.h>

#define JSON_EXT_STRING_FORMAT JSON_C_TO_STRING_PLAIN
#define JSON_EXT_FILE_FORMAT   JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY

#define JSON_EXT_PUT_MODE_FREE -1
#define JSON_EXT_GET_MODE_FREE  0

typedef enum json_ext_type {
	json_str     = 1,
	json_shrt    = 2,
	json_ushrt   = 3,
	json_int     = 4,
	json_uint    = 5,
	json_double  = 6,
	json_arr     = 7,
	json_str_arr = 8,
	json_int_arr = 9,
	json_double_arr = 10,
	json_datetime   = 11,
	json_obj        = 12,
} json_ext_type_t;

typedef struct json_ext_str2 {
	char str[256];
} json_ext_str2_t;

typedef struct json_ext_obj {
	json_ext_type_t t;
	char *name;
	
	union {
		char *str;
		char **str2;
		json_ext_str2_t *str_arr;
		unsigned short *us;
		unsigned short **us_arr;
		short *sh;
		unsigned int *ui;
		int *num;
		int **num2;
		double *dnum;
		double **dnum2;
		struct json_ext_obj *jobj;
		struct json_ext_obj *arr;
	} u;
	
	void *value;
	unsigned int num_arr;
	
	json_object *jobj_arr;
	json_object **tmp;
} json_ext_obj_t;

int json_ext_count_elements_obj(json_ext_obj_t *obj);
void json_ext_print_obj(json_ext_obj_t *obj,unsigned int num_arr);
json_ext_obj_t *json_ext_new_obj(json_ext_obj_t *src);
void json_ext_delete_obj(json_ext_obj_t *obj,int num_arr);

int json_ext_write_file(char *filename,const char *str);
int json_ext_read_file(char *filename,char **outbuffer);

int json_ext_init_value(json_ext_obj_t *obj,char *json_obj_name,void *val,unsigned int num_arr);

void json_ext_put_str(json_object *jobj,char *json_obj_name,char *str);
void json_ext_put_int(json_object *jobj,char *json_obj_name,int *num); 
void json_ext_put_double(json_object *jobj,char *json_obj_name,double *dbnum);
void json_ext_put_array(json_object *jobj,char *json_obj_name,json_ext_obj_t *arr,unsigned int num_arr);
void json_ext_put_obj(json_object *jobj,json_ext_obj_t *obj);
char *json_ext_obj_create(json_ext_obj_t *obj);

char *json_ext_get_str(json_object *jobj,char *json_obj_name);
int *json_ext_get_int(json_object *jobj,char *json_obj_name);
double *json_ext_get_double(json_object *jobj,char *json_obj_name);
void json_ext_get_obj(json_object *jobj,json_ext_obj_t *obj);

int json_ext_clean_array(json_ext_obj_t *obj);
char *json_ext_put_datetime(void *value);
int json_ext_get_datetime(char *str);

int json_ext_find_obj(json_ext_obj_t *obj,int t,char *obj_name);

#endif
