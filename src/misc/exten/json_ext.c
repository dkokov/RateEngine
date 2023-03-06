/*
 * JSON-EXT / several extensions for C application /
 * 
 * https://www.json.org/
 * https://github.com/json-c/json-c
 * https://linuxprograms.wordpress.com/2010/05/20/json-c-libjson-tutorial/
 * 
 * */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../misc/exten/str_ext.h"
#include "../../misc/exten/time_funcs.h"
#include "../../mem/mem.h"

#include "json_ext.h"

int json_ext_count_elements_obj(json_ext_obj_t *obj)
{
	int i;
	
	i=0;
	while(obj[i].t) i++;

	return i;
}

int json_ext_init_value(json_ext_obj_t *obj,char *json_obj_name,void *value,unsigned int num_arr)
{
	int i;
	
	i=0;
	while(obj[i].t) {
		if(strcmp(obj[i].name,json_obj_name) == 0) {
			switch(obj[i].t) {
				case json_str:
					obj[i].u.str = (char *)value;
					break;
				case json_str_arr:
					obj[i].u.str_arr = (json_ext_str2_t *)value;
					break;
				case json_datetime:
					obj[i].u.str = json_ext_put_datetime(value);
					break;
				case json_shrt:
					obj[i].u.sh = (short *)value;
					break;
				case json_ushrt:
					obj[i].u.us = (unsigned short *)value;
					break;
				case json_uint:
					obj[i].u.ui = (unsigned int *)value;
					break;
				case json_int:
					obj[i].u.num = (int *)value;
					break;
				case json_int_arr:
					obj[i].u.num = (int *)value;
					break;
				case json_double:
					obj[i].u.dnum = (double *)value;
					break;
				case json_obj:
					obj[i].u.jobj = (json_ext_obj_t *)value;
					break;
				case json_arr:
					obj[i].num_arr = num_arr;
					obj[i].u.arr = (json_ext_obj_t *)value;
					break;
				default:
					break;
			};

			break;
		}	
				
		i++;		
	}

	return i;
}

void json_ext_print_array(json_ext_obj_t *obj,unsigned int num_arr)
{
	int i,c;
	char **str;
	int **num;
	double **dnum;

	if(obj == NULL) printf("\nobj is NULL\n");
	if(num_arr == 0) printf("\nnum_arr is 0\n");

	json_ext_obj_t *arr = obj->u.arr;

for(c=0;c<num_arr;c++) {
	i=0;
	while(arr[i].t) {
		switch(arr[i].t) {
			case json_str_arr:
				str = arr[i].u.str2;
				printf("arr[%d][%d],%s : %s\n",i,c,arr[i].name,str[c]);
				break;
			case json_int_arr:
				num = arr[i].u.num2;
				printf("arr[%d][%d],%s : %d\n",i,c,arr[i].name,*num[c]);
				break;
			case json_double_arr:
//				printf("arr[%d][%d],%s : %f\n",i,c,obj[i].name,*(double *)arr[i].value);
				dnum = arr[i].u.dnum2;
				if(dnum[c] != NULL) printf("arr[%d][%d],%s : %f\n",i,c,arr[i].name,*dnum[c]);
				break;
			case json_arr:
				if((obj[i].num_arr > 0)) {
//					printf("obj[%d],%s : {%d} array elements \n",i,obj[i].name,obj[i].num_arr);
//					if(num_arr == 0) json_ext_print_obj(&obj[i],obj[i].num_arr);
				}
				break;
			default:
				break;
		};
		
		i++;		
	}	
}
}

void json_ext_print_obj(json_ext_obj_t *obj,unsigned int num_arr)
{
	int i;

	i=0;
	while(obj[i].t) {
		switch(obj[i].t) {
			case json_str:
				if(obj[i].u.str != NULL) printf("obj[%d],%s : %s\n",i,obj[i].name,obj[i].u.str);
				break;
			case json_datetime:
				if(obj[i].u.str != NULL) printf("obj[%d],%s : %s\n",i,obj[i].name,obj[i].u.str);
				break;			
			case json_ushrt:
				if(obj[i].u.us != NULL) printf("obj[%d],%s : %d\n",i,obj[i].name,*obj[i].u.us);				
				break;
			case json_uint:
				if(obj[i].u.ui != NULL) printf("obj[%d],%s : %d\n",i,obj[i].name,*obj[i].u.ui);
				break;
			case json_int:
				if(obj[i].u.num != NULL) printf("obj[%d],%s : %d\n",i,obj[i].name,*obj[i].u.num);
				break;
			case json_double:
				if(obj[i].u.dnum != NULL) printf("obj[%d],%s : %f\n",i,obj[i].name,*obj[i].u.dnum);
				break;
			case json_obj:
				if(obj[i].u.jobj != NULL) json_ext_print_obj(obj[i].u.jobj,0);
				break;
			case json_arr:
				if((obj[i].num_arr > 0)) {
					printf("\tobj[%d],%s : {%d} array elements \n",i,obj[i].name,obj[i].num_arr);
					json_ext_print_array(&obj[i],obj[i].num_arr);
				}
				break;
			default:
				break;
		};
		
		i++;		
	}	
}

json_ext_obj_t *json_ext_new_obj(json_ext_obj_t *src)
{
	int i,num;
	json_ext_obj_t *dst;
	
	num = json_ext_count_elements_obj(src);
	
	dst = calloc(num+1,sizeof(json_ext_obj_t));
	
	if(dst != NULL) {
		i=0;
		while(src[i].t) {
			dst[i].t = src[i].t;
			dst[i].name = strdup(src[i].name);
			
			i++;
		}
	}
	
	return dst;
}

void json_ext_delete_obj(json_ext_obj_t *obj,int num_arr)
{
	int i,c;
	
	if(obj == NULL) return;
	
	i=0;
	while(obj[i].t) {	
		if(obj[i].name != NULL) free(obj[i].name);

		if(num_arr < 0) goto step;

		switch(obj[i].t) {
			case json_str:
				if((obj[i].u.str != NULL)) free(obj[i].u.str);
				obj[i].u.str = NULL;
				break;
			case json_str_arr:
				if((obj[i].u.str2 != NULL)) {
					for(c=0;c<num_arr;c++) free(obj[i].u.str2[c]);
					
					free(obj[i].u.str2);
				}
				obj[i].u.str2 = NULL;
				break;
			case json_ushrt:
				if((obj[i].u.us != NULL)) free(obj[i].u.us);					
				obj[i].u.us = NULL;
				break;
			case json_uint:
				if((obj[i].u.ui != NULL)) free(obj[i].u.ui);
				obj[i].u.ui = NULL;
				break;
			case json_int:
				if((obj[i].u.num != NULL)) free(obj[i].u.num);
				obj[i].u.num = NULL;
				break;
			case json_int_arr:
				if((obj[i].u.num2 != NULL)) {
					for(c=0;c<num_arr;c++) free(obj[i].u.num2[c]);
					
					free(obj[i].u.num2);
				}
				obj[i].u.num2 = NULL;
				break;
			case json_double:
				if((obj[i].u.dnum != NULL)) free(obj[i].u.dnum);
				obj[i].u.dnum = NULL;
				break;
			case json_double_arr:
				if((obj[i].u.dnum2 != NULL)) {
					for(c=0;c<num_arr;c++) free(obj[i].u.dnum2[c]);
					
					free(obj[i].u.dnum2);
				}
				obj[i].u.dnum2 = NULL;
				break;
			case json_obj:
				if(obj[i].u.jobj != NULL) json_ext_delete_obj(obj[i].u.jobj,0);
				obj[i].u.jobj = NULL;
				break;
			case json_arr:
				if((obj[i].u.arr != NULL)) {
					json_ext_delete_obj(obj[i].u.arr,obj[i].num_arr);
				}
				obj[i].u.arr = NULL;
				break;
			default:
				break;
		};

step:
		i++;
	}
	
	free(obj);
}

char *json_ext_get_str(json_object *jobj,char *json_obj_name) 
{
	char *buf;
	const char *tmp_str;
	json_object *tmp;

	tmp = NULL;		
	tmp_str = NULL;
	
	json_object_object_get_ex(jobj,json_obj_name, &tmp);

	if(tmp != NULL) {
		tmp_str = json_object_get_string(tmp);
		
		buf = strdup(tmp_str);
	}

	return buf;
}

unsigned short *json_ext_get_ushrt(json_object *jobj,char *json_obj_name)
{	
	unsigned short *us;
	json_object *tmp;

	tmp = NULL;
	json_object_object_get_ex(jobj,json_obj_name, &tmp);
	if(tmp != NULL) {
		us = (unsigned short *)calloc(1,sizeof(unsigned short));
		
		*us = (unsigned short)json_object_get_int(tmp);	
	}
	
	return us;
}

unsigned int *json_ext_get_uint(json_object *jobj,char *json_obj_name)
{	
	unsigned int *ui;
	json_object *tmp;

	tmp = NULL;
	json_object_object_get_ex(jobj,json_obj_name, &tmp);
	if(tmp != NULL) {
		ui = (unsigned int *)calloc(1,sizeof(unsigned int));
		
		*ui = json_object_get_int(tmp);	
	}
	
	return ui;
}

int *json_ext_get_int(json_object *jobj,char *json_obj_name)
{	
	int *num;
	json_object *tmp;

	tmp = NULL;
	json_object_object_get_ex(jobj,json_obj_name, &tmp);
	if(tmp != NULL) {
		num = (int *)calloc(1,sizeof(int));
		
		*num = json_object_get_int(tmp);	
	}
	
	return num;
}

double *json_ext_get_double(json_object *jobj,char *json_obj_name)
{
	double *dnum;
	json_object *tmp;
	
	tmp = NULL;
	json_object_object_get_ex(jobj,json_obj_name, &tmp);
	if(tmp != NULL) {
		dnum = (double *)calloc(1,sizeof(double));
		
		*dnum = json_object_get_double(tmp);		
	}
	
	return dnum;
}
/*
json_ext_obj_t *json_ext_get_jobject(json_object *jobj,char *json_obj_name)
{
	json_ext_obj_t *obj;
	json_object *tmp;
	
	json_object_object_get_ex(jobj,json_obj_name, &tmp);
	
	return obj;
}*/

int json_ext_get_array(json_object *jobj,char *json_obj_name,json_ext_obj_t *arr)
{
	int i,c;
	unsigned int num_arr;
	char **tmp_str;
	int **tmp_int;
	double **tmp_double;
	json_object *tmp,*tmp_arr_idx;
		
	tmp = NULL;
	num_arr = 0;
	
	if(jobj == NULL) return 0;
	if(arr == NULL) return 0;
	
	json_object_object_get_ex(jobj,json_obj_name, &tmp);

	if(tmp != NULL) {
		num_arr = json_object_array_length(tmp);
		
		for(c=0;c<num_arr;c++) {
			tmp_arr_idx = json_object_array_get_idx(tmp, c);
			
			i = 0;
			while(arr[i].t) {
			
			switch(arr[i].t) {
				case json_str_arr:
					if(arr[i].u.str2 == NULL) {
						arr[i].u.str2 =	calloc(num_arr,sizeof(char *));
					}
					tmp_str = arr[i].u.str2;
					tmp_str[c] = json_ext_get_str(tmp_arr_idx,arr[i].name);
					break;
				case json_int_arr:
					if(arr[i].u.num2 == NULL) {
						arr[i].u.num2 = calloc(num_arr,sizeof(int *));
					}
					tmp_int = arr[i].u.num2;
					tmp_int[c] = json_ext_get_int(tmp_arr_idx,arr[i].name);
					break;
				case json_double_arr:
					if(arr[i].u.dnum2 == NULL) {
						arr[i].u.dnum2 = calloc(num_arr,sizeof(double *));
					}
					tmp_double = arr[i].u.dnum2;
					tmp_double[c] = json_ext_get_double(tmp_arr_idx,arr[i].name);
					break;
				case json_arr:
					//arr[i].value = (void *)
					//json_ext_get_array(tmp_arr_idx,arr[i].name,(json_ext_obj_t *)arr[i].value);
					break;
				default:
					break;
			};
		
			i++;
		}
		
		}
	}

	return num_arr;
}

int json_ext_clean_array(json_ext_obj_t *obj)
{
	int c;
	
	if(obj == NULL) return -1;
	if(obj->num_arr == 0) return -2;
//	if(obj->jobj_arr == NULL) return -3;
	if(obj->tmp == NULL) return -4;
	
	for(c=0;c<obj->num_arr;c++) {
		json_object_put(obj->tmp[c]);
	}
	
	free(obj->tmp);

	return 0;
}

char *json_ext_put_datetime(void *value)
{
	char *buf,*dt;
	
	if(value == NULL) return NULL;
	
	dt = (char *)value;
	
	if(strlen(dt) > 0) {
		buf = str_ext_replace_symbol(dt,' ','T');
		
//		memset(dt,0,strlen(dt));
//		strcpy(dt,buf);
		
//		mem_free(buf);		
	}
	
	return buf;
}

int json_ext_get_datetime(char *str)
{
	char *buf;
	
	if(str == NULL) return -1;
		
	if(strlen(str) > 0) {
		buf = str_ext_replace_symbol(str,'T',' ');
		
		memset(str,0,strlen(str));
		strcpy(str,buf);
		
		mem_free(buf);		
	}
	
	return 0;
}

void json_ext_get_obj(json_object *jobj,json_ext_obj_t *obj)
{
	int i;
	
	if(obj == NULL) return;
	
	if(jobj != NULL) {
		i = 0;
		while(obj[i].t) {			
			switch(obj[i].t) {
				case json_str:
					if(obj[i].u.str == NULL) obj[i].u.str = json_ext_get_str(jobj,obj[i].name);
					break;
				case json_datetime:
					if(obj[i].u.str == NULL) {
						obj[i].u.str = json_ext_get_str(jobj,obj[i].name);
						json_ext_get_datetime(obj[i].u.str);
					}
					break;
				case json_ushrt:
					if(obj[i].u.us == NULL) obj[i].u.us = json_ext_get_ushrt(jobj,obj[i].name);					
					break;
				case json_uint:
					if(obj[i].u.ui == NULL) obj[i].u.ui = json_ext_get_uint(jobj,obj[i].name);
					break;
				case json_int:
					if(obj[i].u.num == NULL) obj[i].u.num = json_ext_get_int(jobj,obj[i].name);
					break;
				case json_double:
					if(obj[i].u.dnum == NULL) obj[i].u.dnum = json_ext_get_double(jobj,obj[i].name);
					break;
				case json_obj:
					//if(obj->jobj_arr == NULL) {
						json_object_object_get_ex(jobj,obj[i].name, &obj->jobj_arr);
						json_ext_get_obj(obj->jobj_arr,obj[i].u.jobj);
					//}
					break;
				case json_arr:
					if(obj[i].u.arr == NULL) obj[i].num_arr = json_ext_get_array(jobj,obj[i].name,obj[i].u.arr);
					break;
				default:
					break;
			};

			i++;
		}
	}
}

int json_ext_find_obj(json_ext_obj_t *obj,int t,char *obj_name)
{
	int i;
	
	i = 0;
	while(obj[i].t) {
		if((obj[i].t == t)&&(strcmp(obj_name,obj[i].name) == 0)) return i;
		
		i++;
	}
	
	return -1;
}

void json_ext_put_str(json_object *jobj,char *json_obj_name,char *str) 
{
	if(jobj != NULL) 
		if((str != NULL)&&(strlen(str) > 0)) json_object_object_add(jobj, json_obj_name, json_object_new_string(str));
}

void json_ext_put_uint(json_object *jobj,char *json_obj_name,unsigned int *ui) 
{
	if(jobj != NULL) 
		if(ui != NULL) json_object_object_add(jobj, json_obj_name, json_object_new_int(*ui));
}

void json_ext_put_int(json_object *jobj,char *json_obj_name,int *num) 
{
	if(jobj != NULL) 
		if(num != NULL) json_object_object_add(jobj, json_obj_name, json_object_new_int(*num));
}

void json_ext_put_ushrt(json_object *jobj,char *json_obj_name,unsigned short *num) 
{
	if(jobj != NULL) 
		if(num != NULL) json_object_object_add(jobj, json_obj_name, json_object_new_int(*num));
}

void json_ext_put_double(json_object *jobj,char *json_obj_name,double *dnum) 
{
	if(jobj != NULL) 
		if(dnum != NULL) json_object_object_add(jobj, json_obj_name, json_object_new_double(*dnum));
}

void json_ext_put_jobject(json_object *jobj,char *json_obj_name,json_ext_obj_t *obj) 
{
	json_object *tmp;
	
	if(obj == NULL) return;
	
	if(jobj != NULL) {
		tmp = json_object_new_object();

		if(tmp != NULL) {
			json_ext_put_obj(tmp,obj);
			json_object_object_add(jobj, json_obj_name, tmp);
		}
	}
}

void json_ext_put_array(json_object *jobj,char *json_obj_name,json_ext_obj_t *arr,unsigned int num_arr)
{
	json_ext_str2_t *ext_str;
	int *num;
	double *dnum;
	int i,c;
	json_object **tmp;
	
	arr->jobj_arr = json_object_new_array();

	tmp = calloc(num_arr,sizeof(json_object *));
	arr->tmp = tmp;
	
	for(c=0;c<num_arr;c++) {
		tmp[c] = json_object_new_object();
		
		i = 0;
		while(arr[i].t) {
			switch(arr[i].t) {
				case json_str_arr:
					ext_str = arr[i].u.str_arr;
					json_ext_put_str(tmp[c],arr[i].name,ext_str[c].str);
					break;
				case json_int_arr:
					num = arr[i].u.num;
					json_ext_put_int(tmp[c],arr[i].name,&num[c]);
					break;
				case json_double:
					dnum = arr[i].u.dnum;
					json_ext_put_double(tmp[c],arr[i].name,&dnum[c]);
					break;
//				case json_arr:
//					json_ext_put_array(tmp,arr[i].name,(json_ext_obj_t *)obj[i].value,obj[i].num_arr);
//					break;
				default:
					break;
			};
		
			i++;
		}
		json_object_array_add(arr->jobj_arr, json_object_get(tmp[c]));
	}
	
	for(c=0;c<num_arr;c++) json_object_put(tmp[c]);
	
	free(tmp);
	
	json_object_object_add(jobj, json_obj_name, arr->jobj_arr);
}

void json_ext_put_obj(json_object *jobj,json_ext_obj_t *obj)
{
	int i;
	
	if(obj == NULL) return;
	
	if(jobj != NULL) {
		i = 0;
		while(obj[i].t) {			
			switch(obj[i].t) {
				case json_str:
					json_ext_put_str(jobj,obj[i].name,obj[i].u.str);
					break;
				case json_datetime:
					json_ext_put_str(jobj,obj[i].name,obj[i].u.str);					
					break;
				case json_ushrt:
					json_ext_put_ushrt(jobj,obj[i].name,obj[i].u.us);
					break;
				case json_int:
					json_ext_put_int(jobj,obj[i].name,obj[i].u.num);
					break;
				case json_uint:
					json_ext_put_uint(jobj,obj[i].name,obj[i].u.ui);
					break;
				case json_double:
					json_ext_put_double(jobj,obj[i].name,obj[i].u.dnum);
					break;
				case json_obj:
					json_ext_put_jobject(jobj,obj[i].name,obj[i].u.jobj);
					break;
				case json_arr:
					json_ext_put_array(jobj,obj[i].name,obj[i].u.arr,obj[i].num_arr);
					break;
				default:
					break;
			};
			
			i++;
		}
	}
}

char *json_ext_obj_create(json_ext_obj_t *obj)
{
	char *buf;
	const char *msg;
	json_object *jobj;
	
	jobj = json_object_new_object();
	
	json_ext_put_obj(jobj,obj);
	
	msg = json_object_to_json_string_ext(jobj,JSON_EXT_STRING_FORMAT);

	buf = (char *)calloc(1,(strlen(msg)+1));

	strcpy(buf,msg);
	
	json_object_put(jobj);

	return buf;
}

int json_ext_write_file(char *filename,const char *str)
{
	FILE* fp = NULL;

	fp = fopen(filename, "w");

	if(fp == NULL) return -1;

	if(strlen(str) == 0) return -2;
	
	fwrite(str,strlen(str),1,fp);
		
	fclose(fp);
	
	return 0;
}

int json_ext_read_file(char *filename,char **outbuffer)
{
	FILE* fp = NULL;
	long filesize;
	size_t readsize;
	char* filebuffer;

	fp = fopen(filename, "r");

	if(fp == NULL) return -1;

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);
	
	filebuffer = (char*) malloc(sizeof(char) * (filesize+1));
	memset(filebuffer,0,(filesize+1));
	*outbuffer = filebuffer;

	if(filebuffer == NULL) return -2;
	
	readsize = fread(filebuffer,1, filesize, fp);
	
	if(readsize != filesize) return -3;
	
	fclose(fp);

	return 0;
}
