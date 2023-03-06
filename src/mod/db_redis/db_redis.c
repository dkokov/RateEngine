/* https://github.com/redis/hiredis/blob/master/hiredis.h */

#include <string.h>

#include <hiredis/hiredis.h>

#include "../../misc/globals.h"
#include "../../db/db.h"

#define REDIS_STATUS_CHK "ping"
#define REDIS_STATUS_OK  "pong"

int db_redis_free_reply(db_conn_t *conn);

int db_redis_connect(db_conn_t *conn)
{
	redisContext *c;
	struct timeval timeout ;

	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	timeout.tv_sec  = conn->timeout;
	timeout.tv_usec = 500000;
	
	c = redisConnectWithTimeout(conn->hostname,conn->port,timeout);
	//c = redisConnectNonBlock(conn->hostname, conn->port);
	//c = redisConnect(conn->hostname,conn->port);
	
	if(c == NULL) return DB_ERR_RCONN_NUL;
	
	//redisSetTimeout(c, timeout);
	
	conn->rconn = (void *)c;

	return DB_OK;
}

int db_redis_close(db_conn_t *conn)
{
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(conn->rconn == NULL) return DB_ERR_RCONN_NUL;
	
	redisFree((redisContext *)conn->rconn);
	conn->rconn = NULL;
	
	return DB_OK;
}

int db_redis_command(db_conn_t *conn,char *command)
{
	int i;
	redisReply *reply;

	db_nosql_result_t *tmp;
	
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(conn->rconn == NULL) return DB_ERR_RCONN_NUL;
	
	reply = redisCommand((redisContext *)conn->rconn,command);
	if(reply == NULL) return DB_ERR_RESULT_NUL;
	
	if((reply->len == 0)&&(reply->elements == 0)) {
		conn->res = NULL;
		conn->result = NULL;
		
		freeReplyObject(reply);
		
		return DB_ERR_NOSQL_RES_NUL;
	}
			
	conn->res = (void *)reply;

	tmp = db_nosql_result_init(reply->elements);

	if(reply->len > 0) {
		tmp->len = reply->len;
		tmp->str = reply->str;
	} else if(reply->elements > 0) {
		tmp->elements = reply->elements;
		
		for(i=0;i<reply->elements;i++) {
			tmp->arr[i].key = reply->element[i]->str;
		}
	} else return DB_ERR_NOSQL_RES_NUL;
	
	conn->result = (void *)tmp;
	
	return DB_OK;
}

int db_redis_set(db_conn_t *conn,char *key,char *json_str)
{
	int ret;
	char buf[DB_BUF_LEN];
	
	db_nosql_result_t *result;

	memset(buf,0,DB_BUF_LEN);
	sprintf(buf,"SET %s '%s'",key,json_str);
	
	ret = db_redis_command(conn,buf);
	if(ret < 0) {
		db_error(ret);
	}
	
	result = (db_nosql_result_t *)conn->result;
	if(result != NULL) {
		DBG("db_redis_set()","command: %s , result: %s",buf,result->str);
		
		if(strncasecmp(result->str,DB_NOSQL_OK_STR,strlen(DB_NOSQL_OK_STR))) {
			db_error_put(DB_ERR_EXT_ERR,result->str);
			return DB_ERR_EXT_ERR;
		}
	}

	return DB_OK;
}

int db_redis_get(db_conn_t *conn,char *key)
{
	int ret;
	char buf[DB_BUF_LEN];
	
	memset(buf,0,DB_BUF_LEN);
	sprintf(buf,"GET %s",key);
	
	ret = db_redis_command(conn,buf);
	if(ret < 0) {
		db_error(ret);
	}
	
	return DB_OK;
}

int db_redis_status(db_conn_t *conn)
{
	int ret;
	db_nosql_result_t *result;
	
	ret = db_redis_command(conn,REDIS_STATUS_CHK);

	if(ret == DB_OK) {
		result = (db_nosql_result_t *)conn->result;
		
		if(strncasecmp(result->str,REDIS_STATUS_OK,strlen(REDIS_STATUS_OK)) == 0) ret = DB_OK;
		else ret = DB_ERR_CONN_NOOK;
		
		db_redis_free_reply(conn);
		
		db_nosql_result_free(result);
		conn->result = NULL;
	} else ret = DB_ERR_CONN_NOOK;

	return ret;
}

int db_redis_free_reply(db_conn_t *conn)
{
	if(conn == NULL) return DB_ERR_CONN_NUL;
	
	if(conn->res != NULL) {
		freeReplyObject((redisReply *)conn->res);
		conn->res = NULL;
	}
	
	return DB_OK;
}

int db_redis_bind_api(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	
	ptr->t = nosql;
	
	ret = db_init_t(ptr);
	
	if(ret < 0) return ret;
	
	ptr->u.nosql->connect    = db_redis_connect;
	ptr->u.nosql->command    = db_redis_command;
	ptr->u.nosql->set        = db_redis_set;
	ptr->u.nosql->get        = db_redis_get;
	ptr->u.nosql->status     = db_redis_status;
	ptr->u.nosql->close      = db_redis_close;
	ptr->u.nosql->free_reply = db_redis_free_reply;
		
	return DB_OK;
}
