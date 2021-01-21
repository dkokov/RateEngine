/*
 * http://mongoc.org/libmongoc/current/tutorial.html
 * http://mongoc.org/libbson/current/json.html
 */

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include "../../misc/globals.h"
#include "../../db/db.h"

#define MONGODB_PORT    21017
#define MONGODB_HOST    "localhost"
#define MONGODB_APPNAME "db_mongodb"

int db_mongodb_status(db_conn_t *conn);

bson_t *db_mongodb_json_to_bson(char *json_str)
{
	bson_t *new;
	bson_error_t error;
	
	new = NULL;
	new = bson_new_from_json ((const uint8_t *)json_str,strlen(json_str),&error);
	
	if(new == NULL) LOG("db_mongodb_json_to_bson()","LibBSON error: %s",error.message);
	
	return new;
}

char *db_mongodb_bson_to_json(bson_t *bson_ptr)
{
	return bson_as_json(bson_ptr, NULL);
}

int db_mongodb_connect(db_conn_t *conn)
{
	char buf[DB_BUF_LEN];	
	mongoc_uri_t *uri;
	mongoc_client_t *c;
	bson_error_t error;
   
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(strlen(conn->hostname) == 0) strcpy(conn->hostname,MONGODB_HOST);
	if(conn->port == 0) conn->port = MONGODB_PORT;	
	
	mongoc_init();

	memset(buf,0,DB_BUF_LEN);
	sprintf(buf,"mongodb://%s:%d/?appname=%s",conn->hostname,conn->port,MONGODB_APPNAME);
	
	uri = mongoc_uri_new_with_error(buf,&error);
	if(!uri) {		
		db_error_put(DB_ERR_EXT_ERR,error.message);
		return DB_ERR_EXT_ERR;
	}
	
	//c = mongoc_client_new(buf);
	c = mongoc_client_new_from_uri(uri);
//   if (!client) {
//      return EXIT_FAILURE;
//   }
	if(c == NULL) {
		mongoc_cleanup();
		return DB_ERR_RCONN_NUL;
	}
	
	conn->rconn = (void *)c;
	mongoc_uri_destroy(uri);
	
	if(db_mongodb_status(conn) == DB_OK) return DB_OK;
	else return DB_ERR_CONN_NOOK;
}

int db_mongodb_close(db_conn_t *conn)
{
	if(conn == NULL) return DB_ERR_CONN_NUL;
	if(conn->rconn == NULL) return DB_ERR_RCONN_NUL;
	
	mongoc_client_destroy((mongoc_client_t *)conn->rconn);
	conn->rconn = NULL;
	
	mongoc_cleanup();	
	
	return DB_OK;
}

int db_mongodb_status(db_conn_t *conn)
{
	bson_t *ping,reply;
	bson_error_t error;
	char *str;
	bool retval;
	      
	ping = BCON_NEW ("ping", BCON_INT32 (1));

	retval = mongoc_client_command_simple((mongoc_client_t *)conn->rconn, "admin", ping, NULL, &reply, &error);

	if(!retval) {
		LOG("db_mongodb_status()","Error: %s", error.message);
		return -1;
	}

	str = bson_as_json(&reply, NULL);
	LOG("db_mongodb_status()","Result: %s\n", str);
	
	bson_destroy(&reply);
	bson_destroy(ping);
	bson_free (str);

	return DB_OK;
}

int db_mongodb_command(db_conn_t *conn,char *command)
{
/*	int i;
	bson_t reply;

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
	
	conn->result = (void *)tmp;*/
	
	return DB_OK;
}

int db_mongodb_set(db_conn_t *conn,char *coll,char *json_str)
{
	int ret;
	
	mongoc_collection_t *collection;
	bson_error_t error;
	bson_t *insert;
	
	ret = DB_OK;
	
	collection = mongoc_client_get_collection((mongoc_client_t *)conn->rconn,conn->dbname,coll);
	if(collection != NULL) {
		insert = db_mongodb_json_to_bson(json_str);
		if(insert != NULL) {
			if (!mongoc_collection_insert_one (collection, insert, NULL, NULL, &error)) {
				LOG("db_mongodb_set()", "Error: %s", error.message);
			}

			bson_destroy(insert);
		} else ret = DB_ERR_EXT_ERR;
	
		mongoc_collection_destroy (collection);	
	}
	
	return ret;
}

int db_mongodb_get(db_conn_t *conn,char *coll)
{
	int ret;
	
	mongoc_collection_t *collection;
	mongoc_cursor_t *cursor;
	bson_error_t error;
	bson_t *doc;
	char *str;
	bson_t *query;
	
	collection = mongoc_client_get_collection((mongoc_client_t *)conn->rconn,conn->dbname,coll);

   query = BCON_NEW ("$query",
                     "{",
                     "foo",
                     BCON_INT32 (1),
                     "}",
                     "$orderby",
                     "{",
                     "bar",
                     BCON_INT32 (-1),
                     "}");
   
	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
	while(mongoc_cursor_next(cursor, doc)) {
		str = db_mongodb_bson_to_json(doc);
		printf ("%s\n", str);
		bson_free (str);
	}

	if (mongoc_cursor_error (cursor, &error)) {
		fprintf (stderr, "An error occurred: %s\n", error.message);
	}

   mongoc_cursor_destroy (cursor);
   bson_destroy (query);
	
	return ret;
}

int db_mongodb_bind_api(db_t *ptr)
{
	int ret;
	
	if(ptr == NULL) return DB_ERR_DBP_NUL;
	
	ptr->t = nosql;
	
	ret = db_init_t(ptr);
	
	if(ret < 0) return ret;
	
	ptr->u.nosql->connect    = db_mongodb_connect;
	ptr->u.nosql->command    = db_mongodb_command;
	ptr->u.nosql->set        = db_mongodb_set;
	ptr->u.nosql->status     = db_mongodb_status;
	ptr->u.nosql->close      = db_mongodb_close;
	ptr->u.nosql->free_reply = NULL;
		
	return DB_OK;
}
