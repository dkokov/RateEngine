#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "duckdb.h"

#include "../../misc/globals.h"
#include "../../mem/mem.h"
#include "../../db/db.h"

#include "db_duckdb.h"

/*
 * DuckDB as a first-class db engine (peer of pgsql/mysql/mongodb).
 *
 * enginename "duckdb" -> db_engine_bind() dlopen's duckdb.so and calls
 * db_duckdb_bind_api(), which fills the db_sql_t vtable below.
 *
 * Connection model:
 *   conn->rconn  -> db_duckdb_ctx_t* (the duckdb database + connection)
 *   conn->res    -> duckdb_result*  (native result of the last query)
 *   conn->result -> db_sql_result_t* (marshaled rows-as-strings, like pgsql)
 *
 * conn->dbname selects the DuckDB store: empty or ":memory:" -> in-memory,
 * otherwise a file path. host/user/pass/port are unused (embedded engine);
 * attaching an external DB is done by the caller via SQL (ATTACH ...).
 */

int db_duckdb_connect(db_conn_t *conn)
{
	db_duckdb_ctx_t *ctx;
	const char *path;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	ctx = (db_duckdb_ctx_t *)mem_alloc(sizeof(db_duckdb_ctx_t));
	if(ctx == NULL) return DB_ERR_RCONN_NUL;

	/* dbname acts as the DuckDB path; empty/":memory:" -> in-memory */
	path = (strlen(conn->dbname) > 0) ? conn->dbname : NULL;
	if(path != NULL && strcmp(path,":memory:") == 0) path = NULL;

	if(duckdb_open(path,&ctx->db) != DuckDBSuccess) {
		LOG("db_duckdb_connect()","duckdb_open failed");
		mem_free(ctx);
		return DB_ERR_CONN_NOOK;
	}

	if(duckdb_connect(ctx->db,&ctx->conn) != DuckDBSuccess) {
		LOG("db_duckdb_connect()","duckdb_connect failed");
		duckdb_close(&ctx->db);
		mem_free(ctx);
		return DB_ERR_CONN_NOOK;
	}

	conn->rconn = (void *)ctx;

	return DB_OK;
}

int db_duckdb_query(db_conn_t *conn,char *query)
{
	db_duckdb_ctx_t *ctx;
	duckdb_result *res;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	ctx = (db_duckdb_ctx_t *)conn->rconn;
	if(ctx == NULL) return DB_ERR_RCONN_NUL;

	res = (duckdb_result *)mem_alloc(sizeof(duckdb_result));
	if(res == NULL) return DB_ERR_SQLRES_NUL;

	if(duckdb_query(ctx->conn,query,res) == DuckDBError) {
		LOG("db_duckdb_query()","query error: %s",duckdb_result_error(res));
		duckdb_destroy_result(res);
		mem_free(res);
		return DB_ERR_SQLEXEC_NOOK;
	}

	DBG("db_duckdb_query()","query '%s'",query);

	conn->res = (void *)res;

	return DB_OK;
}

/*
 * Marshal the native duckdb_result into the generic db_sql_result_t.
 * duckdb_value_varchar() renders any column type to text (caller frees it),
 * which keeps every engine's result shape identical (rows of strings).
 * Copies are bounds-checked into the fixed COL_NAME_LEN/ROW_LEN buffers,
 * so over-long values are truncated rather than overflowing.
 */
int db_duckdb_fetch_array(db_conn_t *conn)
{
	idx_t r,c;
	int rows,cols;

	duckdb_result *res;
	db_sql_result_t *sql_res;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;

	res = (duckdb_result *)conn->res;

	cols = (int)duckdb_column_count(res);
	rows = (int)duckdb_row_count(res);

	sql_res = mem_alloc(sizeof(db_sql_result_t));

	if(sql_res == NULL) return DB_ERR_RESULT_NUL;

	sql_res->rows = rows;
	sql_res->cols = cols;

	if(db_sql_result_init(sql_res) < 0) return DB_ERR_RESULT_NUL;

	for(c = 0; c < (idx_t)cols; c++) {
		const char *cname = duckdb_column_name(res,c);

		if(cname != NULL) {
			strncpy(sql_res->cols_list[c].col_name,cname,COL_NAME_LEN-1);
			sql_res->cols_list[c].col_name[COL_NAME_LEN-1] = '\0';
		}

		for(r = 0; r < (idx_t)rows; r++) {
			char *val = duckdb_value_varchar(res,c,r);

			if(val != NULL) {
				strncpy(sql_res->cols_list[c].rows_list[r].row,val,ROW_LEN-1);
				sql_res->cols_list[c].rows_list[r].row[ROW_LEN-1] = '\0';
				duckdb_free(val);
			} else {
				sql_res->cols_list[c].rows_list[r].row[0] = '\0';
			}
		}
	}

	conn->result = (void *)sql_res;

	return DB_OK;
}

int db_duckdb_status(db_conn_t *conn)
{
	db_duckdb_ctx_t *ctx;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	ctx = (db_duckdb_ctx_t *)conn->rconn;
	if(ctx == NULL) return DB_ERR_RCONN_NUL;

	/* embedded engine: a live connection handle means it is up */
	return DB_OK;
}

void db_duckdb_close(db_conn_t *conn)
{
	db_duckdb_ctx_t *ctx;

	if(conn == NULL) return;

	ctx = (db_duckdb_ctx_t *)conn->rconn;
	if(ctx == NULL) return;

	duckdb_disconnect(&ctx->conn);
	duckdb_close(&ctx->db);

	mem_free(ctx);
	conn->rconn = NULL;
}

int db_duckdb_free_result(db_conn_t *conn)
{
	duckdb_result *res;

	if(conn == NULL) return DB_ERR_CONN_NUL;

	if(conn->res == NULL) return DB_ERR_SQLRES_NUL;

	res = (duckdb_result *)conn->res;

	duckdb_destroy_result(res);
	mem_free(res);

	conn->res = NULL;

	return DB_OK;
}

int db_duckdb_bind_api(db_t *ptr)
{
	int ret;

	if(ptr == NULL) return DB_ERR_DBP_NUL;

	ptr->t = sql;

	ret = db_init_t(ptr);

	if(ret < 0) return ret;

	ptr->u.sql->connect     = db_duckdb_connect;
	ptr->u.sql->query       = db_duckdb_query;
	ptr->u.sql->fetch       = db_duckdb_fetch_array;
	ptr->u.sql->status      = db_duckdb_status;
	ptr->u.sql->close       = db_duckdb_close;
	ptr->u.sql->free_result = db_duckdb_free_result;

	return DB_OK;
}
