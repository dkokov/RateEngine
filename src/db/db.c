#include <stdio.h>
#include <string.h>
#include <dlfcn.h> 
#include <unistd.h>

#include "../misc/globals.h"
#include "../mem/mem.h"

#include "db.h"

/* DB errors list - global array */
err_t db_err[] = {
	{DB_ERR_DBP_NUL,"DB interface struct pointer is NULL (cannot init the struct)!"},
	{DB_ERR_CONN_NUL,"DB connection struct pointer is NULL !"},
	{DB_ERR_CONN_NOOK,"DB server is disconnected !"},
	{DB_ERR_RCONN_NUL,"DB interfaces struct,a pointer 'rconn' is NULL !"},
	{DB_ERR_RCONN_NOOK,"DB server is not reconnected successful !"},
	{DB_ERR_ENG_NUL,"DB ENGINE pointer is NULL !"},
	{DB_ERR_ENGNAME_NUL,"DB ENGINE NAME is empty (empty string / NULL) !"},
	{DB_ERR_ENGBIND_NUL,"DB ENGINE is not binding successful !"},
	{DB_ERR_SQL_NUL,"DB SQL struct pointer is NULL (cannot init the struct) !"},
	{DB_ERR_SQLRES_NUL,"DB ENGINE SQL result pointer is NULL !"},
	{DB_ERR_SQLCOLS_NUL,""},
	{DB_ERR_SQLEXEC_NOOK,"DB SQL Exec is not OK (the server is returned errors)!"},
	{DB_ERR_NOSQL_NUL,"DB NOSQL pointer is NULL (cannot init the struct) !"},
	{DB_ERR_TYPE_UNK,"DB interface invalid type !"},
	{DB_ERR_FUNC_UNK,"DB interface invalid API (calling function) !"},
	{DB_ERR_FUNC_NUL,"DB interface API pointer is NULL !"},
	{DB_ERR_RESULT_NUL,"DB interface result pointer is NULL !"},
	{DB_ERR_EXT_ERR,"..."},
	{ERROR_END,""}
};

db_t *db_init(void)
{
	return mem_alloc(sizeof(db_t));
}

int db_init_t(db_t *ptr)
{
	db_sql_t *sql_ptr;
	db_nosql_t *nosql_ptr;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;

	sql_ptr   = NULL;
	nosql_ptr = NULL;

	switch(ptr->t) {
		case sql:
			sql_ptr = mem_alloc(sizeof(db_sql_t));

			if(sql_ptr != NULL) ptr->u.sql = sql_ptr;
			else return DB_ERR_SQL_NUL;

			break;
		case nosql:
			nosql_ptr = mem_alloc(sizeof(db_nosql_t));

			if(nosql_ptr != NULL) ptr->u.nosql = nosql_ptr;
			else return DB_ERR_NOSQL_NUL;

			break;
		default:
			return DB_ERR_TYPE_UNK;
	}
	
	return DB_OK;
}

void db_free(db_t *ptr)
{
	if(ptr != NULL) {
		switch(ptr->t) {
			case sql:
				mem_free(ptr->u.sql);
				break;
			case nosql:
				mem_free(ptr->u.nosql);
				break;
			default:
				break;
		}
		
		if(ptr->conn != NULL) {
			if(ptr->conn->res != NULL) db_free_result(ptr);
			
			if(ptr->conn->result != NULL) {
				if(ptr->t == sql) db_sql_result_free((db_sql_result_t *)ptr->conn->result);
				if(ptr->t == nosql) db_nosql_result_free((db_nosql_result_t *)ptr->conn->result);
			}
			
			db_conn_free(ptr->conn);
		}
		
		mem_free(ptr);
	}
}

int db_engine_bind(db_t *ptr)
{	
	int ret;
	void *func;

	char libname[255];
	char funcname[255];
	
	db_conn_t *conn;
	mod_t *mod_ptr;
	
	int (*fptr)(db_t *);

	if(ptr == NULL) return DB_ERR_DBP_NUL;
	
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	conn = ptr->conn;
	
	if(strlen(conn->enginename) == 0) return DB_ERR_ENGNAME_NUL;
	
	ret = DB_ERR_ENGBIND_NUL;
	
	memset(libname,0,255);
	sprintf(libname,"%s.so",conn->enginename);
	
	memset(funcname,0,255);
	sprintf(funcname,"db_%s_bind_api",conn->enginename);
	
	mod_ptr = mod_find_module(libname);
	if(mod_ptr != NULL) {
		if(mod_ptr->handle != NULL) {

			func = mod_find_sim(mod_ptr->handle,funcname);		
			if(func != NULL) {
				fptr = func;
			
				ret = fptr(ptr);
			}
		}
	}
	
	return ret;
}

db_conn_t *db_conn_init(char *enginename,char *host,char *dbname,unsigned short port,char *usr,char *pass,unsigned short timeout)
{
	db_conn_t *conn;
		
	conn = mem_alloc(sizeof(db_conn_t));
	
	if(conn != NULL) {
		strcpy(conn->enginename,enginename);
		strcpy(conn->hostname,host);
		strcpy(conn->dbname,dbname);
		conn->port = port;
		strcpy(conn->username,usr);
		strcpy(conn->password,pass);
		conn->timeout = timeout;
	}
	
	return conn;
}

void db_conn_free(db_conn_t *conn)
{
	mem_free(conn);
}

db_nosql_result_t *db_nosql_result_init(int n)
{
	db_nosql_result_t *tmp;
	
	tmp = (db_nosql_result_t *)mem_alloc(sizeof(db_nosql_result_t));

	if((tmp != NULL)&&(n > 0)) {
		tmp->elements = n;
		tmp->arr = (db_nosql_pair_t *)mem_alloc_arr((n+1),sizeof(db_nosql_pair_t));
	}

	return tmp;
}

void db_nosql_result_free(db_nosql_result_t *ptr)
{
	if(ptr != NULL) {
		if(ptr->arr != NULL ) mem_free(ptr->arr);
		
		mem_free(ptr);
		ptr = NULL;
	}
}

db_sql_row_t *db_sql_rows_init(int rows)
{	
	return mem_alloc_arr(rows,sizeof(db_sql_row_t));;
}

int db_sql_result_init(db_sql_result_t *ptr)
{
	int i;
	
	if(ptr == NULL) return DB_ERR_SQLRES_NUL;
	
	if(ptr->cols > 0) { 
		ptr->cols_list = mem_alloc_arr(ptr->cols,sizeof(db_sql_col_name_t));
		
		if(ptr->cols_list == NULL) return DB_ERR_SQLCOLS_NUL;

		if(ptr->rows > 0) { 
			for(i=0;i<ptr->cols;i++) {
				ptr->cols_list[i].rows_list = mem_alloc_arr(ptr->rows,sizeof(db_sql_row_t));
			}
		}
	}
		
	return DB_OK;
}

void db_sql_result_free(db_sql_result_t *ptr)
{
	int i;
	
	if(ptr != NULL) {
		//if(ptr->rows > 0) { BUG#10 & BUG#23 ???
			if(ptr->cols_list != NULL) {	
				for(i=0;i<ptr->cols;i++) {
					if(ptr->cols_list[i].rows_list != NULL) {
						mem_free(ptr->cols_list[i].rows_list);
						ptr->cols_list[i].rows_list = NULL;
					}
				}
			
				mem_free(ptr->cols_list);
				ptr->cols_list = NULL;
			}
		//}
		
		mem_free(ptr);
		ptr = NULL;
	}
}

int db_connect(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	switch(ptr->t) {
		case sql:
			if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
			if(ptr->u.sql->connect == NULL) return DB_ERR_FUNC_NUL;
			
			ret = ptr->u.sql->connect(ptr->conn);			
			break;
		case nosql:
			if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
			if(ptr->u.nosql->connect == NULL) return DB_ERR_FUNC_NUL;
			
			ret = ptr->u.nosql->connect(ptr->conn);
			break;
		default:
			return DB_ERR_TYPE_UNK;
	}
	
	return ret;
}

int db_query(db_t *ptr,char *query,int clear)
{	
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == sql) {
		if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
		if(ptr->u.sql->query == NULL) return DB_ERR_FUNC_NUL;
			
		ret = ptr->u.sql->query(ptr->conn,query);		
	} else ret = DB_ERR_TYPE_UNK;

	if(clear) db_free_result(ptr);

	return ret;
}

int db_select(db_t *ptr,char *query)
{	
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == sql) {
		if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
		if(ptr->u.sql->query == NULL) return DB_ERR_FUNC_NUL;

		if(strncasecmp(query,DB_SQL_SELECT_STR,strlen(DB_SQL_SELECT_STR)) == 0) {
			
			ret = ptr->u.sql->query(ptr->conn,query);		

			return ret; 
		} else return DB_ERR_FUNC_UNK;
	} else return DB_ERR_TYPE_UNK;
}

int db_fetch(db_t *ptr)
{	
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == sql) {
		if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
		if(ptr->u.sql->fetch == NULL) return DB_ERR_FUNC_NUL;
	
		ret = ptr->u.sql->fetch(ptr->conn);
		
		if(ret == DB_OK) db_free_result(ptr);
		
		return ret;
	} else return DB_ERR_TYPE_UNK;
}

int db_status(db_t *ptr)
{	
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	switch(ptr->t) {
		case sql:
			if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
			if(ptr->u.sql->status == NULL) return DB_ERR_FUNC_NUL;
			
			ret = ptr->u.sql->status(ptr->conn);			
			break;
		case nosql:
			if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
			if(ptr->u.nosql->status == NULL) return DB_ERR_FUNC_NUL;		
		
			ret = ptr->u.nosql->status(ptr->conn);
			break;
		default:
			ret = DB_ERR_TYPE_UNK;
	}

	return ret;
}

int db_free_result(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->conn->res == NULL) return DB_OK;
	
	switch(ptr->t) {
		case sql:
			if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
			if(ptr->u.sql->free_result == NULL) return DB_ERR_FUNC_NUL;
			
			ret = ptr->u.sql->free_result(ptr->conn);
			break;
		case nosql:
			if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
			if(ptr->u.nosql->free_reply == NULL) return DB_ERR_FUNC_NUL;		
		
			ret = ptr->u.nosql->free_reply(ptr->conn);
			break;
		default:
			ret = DB_ERR_TYPE_UNK;
	}
		
	return ret;
}

int db_insert(db_t *ptr,char *query)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	 
	if(ptr->t == sql) {
		if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
		if(ptr->u.sql->query == NULL) return DB_ERR_FUNC_NUL;

		if(strncasecmp(query,DB_SQL_INSERT_STR,strlen(DB_SQL_INSERT_STR)) == 0) {
			ret = ptr->u.sql->query(ptr->conn,query);
		
			if(ret == DB_OK) db_free_result(ptr);
		
			return ret;
		} else return DB_ERR_FUNC_UNK;
	} else return DB_ERR_TYPE_UNK;	
}

int db_update(db_t *ptr,char *query)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == sql) {
		if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
		if(ptr->u.sql->query == NULL) return DB_ERR_FUNC_NUL;
		
		if(strncasecmp(query,DB_SQL_UPDATE_STR,strlen(DB_SQL_UPDATE_STR)) == 0) {
			ret = ptr->u.sql->query(ptr->conn,query);
		
			if(ret == DB_OK) db_free_result(ptr);		
		
			return ret; 
		} else return DB_ERR_FUNC_UNK;
	} else return DB_ERR_TYPE_UNK;	
}

int db_command(db_t *ptr,char *str)
{	
	int ret,n,c;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	n = 5;c = 5;
	
	if(ptr->t == nosql) {
		if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
		if(ptr->u.nosql->command == NULL) return DB_ERR_FUNC_NUL;
		
		again:
		if(c == 0) return DB_ERR_CONN_NOOK;
		
		ret = ptr->u.nosql->command(ptr->conn,str);
		if(ret < 0) {
			LOG("db_command()","ret: %d,c: %d",ret,c);

				loop:
				if(n == 0) return DB_ERR_CONN_NOOK;  
				
				db_free_result(ptr);
				db_close(ptr);
				
				usleep(10000);

				ret = db_connect(ptr);
				if(ret < 0) {
					db_error(ret);
					LOG("db_command()","after db_reconnect(),n:%d",n);
					n--;
					goto loop;
				} else {
					ret = db_status(ptr);
					
					LOG("db_command()","db_status(),ret:%d",ret);

					if(ret < 0) db_error(ret);
					
					c--;		
					goto again;
				}
		}
		
		return ret;
	} else return DB_ERR_TYPE_UNK;	
}

int db_command_2(db_t *ptr,char *str,...)
{
	va_list ap;
	char va_msg[DB_BUF_LEN] = {0};

	va_start(ap,str);
	
	if(strchr(str,'%') != NULL) vsprintf(va_msg,str,ap);
	else strcpy(va_msg,str);
	
	db_command(ptr,va_msg);
	
	va_end(ap);
	
	return 0;
}
/*
int db_set(db_t *ptr,char *str)
{	
	int ret;
	db_nosql_result_t *result;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == nosql) {
		if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
		if(ptr->u.nosql->command == NULL) return DB_ERR_FUNC_NUL;

		if(strncasecmp(str,DB_NOSQL_SET_STR,strlen(DB_NOSQL_SET_STR)) == 0) {
			ret = db_command(ptr,str);

			result = (db_nosql_result_t *)ptr->conn->result;

			if(ret == DB_OK) {	
				if(strncasecmp(result->str,DB_NOSQL_OK_STR,strlen(DB_NOSQL_OK_STR)) == 0) ret = DB_OK;
				else {
					db_error_put(DB_ERR_EXT_ERR,result->str);
					ret = DB_ERR_EXT_ERR;
				}
			}
			
			db_free_result(ptr);
			
			db_nosql_result_free(result);
			ptr->conn->result = NULL;	

			return ret;
		} else return DB_ERR_FUNC_UNK;
	} else return DB_ERR_TYPE_UNK;
}*/

int db_set(db_t *ptr,char *key,char *value)
{	
	int ret;
	db_nosql_result_t *result;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == nosql) {
		if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
		if(ptr->u.nosql->set == NULL) return DB_ERR_FUNC_NUL;
			ret = ptr->u.nosql->set(ptr->conn,key,value);

			result = (db_nosql_result_t *)ptr->conn->result;
/*
			if(ret == DB_OK) {	
				if(strncasecmp(result->str,DB_NOSQL_OK_STR,strlen(DB_NOSQL_OK_STR)) == 0) ret = DB_OK;
				else {
					db_error_put(DB_ERR_EXT_ERR,result->str);
					ret = DB_ERR_EXT_ERR;
				}
			}
	*/		
			db_free_result(ptr);
			
			db_nosql_result_free(result);
			ptr->conn->result = NULL;	

			return ret;
	} else return DB_ERR_TYPE_UNK;
}

int db_get(db_t *ptr,char *key)
{	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;
	
	if(ptr->t == nosql) {
		if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
		if(ptr->u.nosql->get == NULL) return DB_ERR_FUNC_NUL;

			return ptr->u.nosql->get(ptr->conn,key);	
	} else return DB_ERR_TYPE_UNK;
}

int db_close(db_t *ptr)
{	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	if(ptr->conn == NULL) return DB_ERR_CONN_NUL;

	switch(ptr->t) {
		case sql:
			if(ptr->u.sql == NULL) return DB_ERR_SQL_NUL;
			if(ptr->u.sql->close == NULL) return DB_ERR_FUNC_NUL;

			ptr->u.sql->close(ptr->conn);
			break;
		case nosql:
			if(ptr->u.nosql == NULL) return DB_ERR_NOSQL_NUL;
			if(ptr->u.nosql->close == NULL) return DB_ERR_FUNC_NUL;		
		
			ptr->u.nosql->close(ptr->conn);
			break;
		default:
			return DB_ERR_TYPE_UNK;
	}
	
	return DB_OK;
}

void db_error(int error)
{
	int i = 0;
	
	while(db_err[i].error_num < 0) {
		if(db_err[i].error_num == error) {
			LOG("db_error()","[%d][%s]",db_err[i].error_num,db_err[i].error_str);
			break;
		}
		
		i++;
	}
}

void db_error_put(int error,char *ext_err)
{
	int i = 0;
	
	while(db_err[i].error_num < 0) {
		if(db_err[i].error_num == error) {
//			LOG("db_error()","[%d][%s]",db_err[i].error_num,db_err[i].error_str);
			strcpy(db_err[i].error_str,ext_err);
			break;
		}
		
		i++;
	}
}

void db_test(void)
{
	int i,ret;
	
	char sql_str[] = "select * from version";
	
	db_t *dbp;
	db_sql_result_t *result;
	db_nosql_result_t *tmp;
	
	dbp = db_init();
	
	dbp->conn = db_conn_init(mcfg->dbtype,mcfg->dbhost,mcfg->dbname,mcfg->dbport,mcfg->dbuser,mcfg->dbpass,1);
	
	if(dbp->conn == NULL) goto _end;
	
	ret = db_engine_bind(dbp);
	if(ret < 0) {
		LOG("db_test()","DB Test,db_engine_bind() error !");
		db_error(ret);
		goto _end;
	}
	
	ret = db_connect(dbp);
	if(ret < 0) {
		LOG("db_test()","DB Test,db_connect() error !");
		db_error(ret);
		goto _end;
	}
	
	if(dbp->t == sql) {
		/* DB SQL test */
		db_query(dbp,"begin",1);
		
		ret = db_select(dbp,sql_str);
		if(ret < 0) {
			LOG("db_test()","DB Test,db_select() error !");
			db_error(ret);
			goto _end;
		}
	
		ret = db_fetch(dbp);
		if(ret < 0) {
			db_error(ret);
			goto _end;
		}
	
		if(dbp->conn->result != NULL) {
			result = (db_sql_result_t *)dbp->conn->result;
		
			if(result->rows >= 1) {
				LOG("db_test()","DB Test, SQL Query: '%s' /%s,%s/",sql_str,result->cols_list[0].col_name,result->cols_list[1].col_name);
				
				for(i=0;i<result->rows;i++) {
					LOG("db_test()","DB Test, result: %s, %s",result->cols_list[0].rows_list[i].row,result->cols_list[1].rows_list[i].row);
				}
			}
		
			db_sql_result_free(result);
			dbp->conn->result = NULL;
		}
		
		db_query(dbp,"commit",1);		
	} else if(dbp->t == nosql) {
		/* DB NOSQL test */
		//ret = db_set(dbp,"set cld '359888362134%D1%8E'");
		ret = db_set(dbp,"t2","{\"clg\":\"359888362134\"}");
		if(ret < 0) {
			LOG("db_test()","DB Test,db_set() error !");
			
			db_error(ret);
			goto _end;
		} 
		
		ret = db_get(dbp,"t2");
		if(ret < 0) {
			LOG("db_test()","DB Test,db_get() error !");
			
			db_error(ret);
			goto _end;
		} else {
			if(dbp->conn->result != NULL) {
				tmp = (db_nosql_result_t *)dbp->conn->result;

				LOG("db_test()","DB Test,db_get() result: %s",tmp->str);
				db_free_result(dbp);
			
				db_nosql_result_free(tmp);
				dbp->conn->result = NULL;
			}
		}
		ret = 1;
		//ret = db_command_2(dbp,"keys %s","cdrm_*");
		if(ret < 0) {
			LOG("db_test()","DB Test,db_command() error !");
			
			db_error(ret);
			goto _end;
		} else {
				if(dbp->conn->result != NULL) {

				tmp = (db_nosql_result_t *)dbp->conn->result;
			
				for(int i=0;i<tmp->elements;i++) LOG("db_test()","DB Test,db_command() result: %s",tmp->arr[i].key);

				db_free_result(dbp);
			
				db_nosql_result_free(tmp);
				dbp->conn->result = NULL;
			}
		}
				
		ret = db_status(dbp);
		if(ret < 0) {
			LOG("db_test()","DB Test,db_status() error !");
			
			db_error(ret);
			goto _end;
		} else {
			if(dbp->conn->result != NULL) {
				tmp = (db_nosql_result_t *)dbp->conn->result;
			
				LOG("db_test()","DB Test,db_status() result: %s",tmp->str);
				db_free_result(dbp);
			
				db_nosql_result_free(tmp);
				dbp->conn->result = NULL;
			}
		}
	} else {
		/* Error !!! */
		LOG("db_test()","Unk db type !");
	}

_end:
	db_close(dbp);
	db_free(dbp);	
}
