# RateEngine V7 — Core Architecture & Module Conventions

> **Read this first.** It is the orientation doc for any work (human or AI) on RateEngine V7.
> It describes the core abstraction layers every module must use, the conventions to
> follow, and the gotchas that bite. Do not reinvent memory, threads, networking, DB
> access, config, or logging — the layers below already provide them.

RateEngine V7 is a telecom billing/rating engine in C, built as a **modular plugin
system**: a small core library (`libre7core`) plus `.so` modules loaded at runtime via
`dlopen`. Modules discover each other and the core through **function-pointer tables**
("bind APIs"). The core never hard-links a module; everything is bound at runtime.

```
libre7core  ──  mem · proc · net · db · config · log · mod   (compiled once, linked by every module)
   │
   ├── transports:  tcp.so  udp.so  tls.so  sctp.so           (net vtable)
   ├── db engines:  db_pgsql.so db_mysql.so db_redis.so
   │                db_mongodb.so db_duckdb.so                 (db vtable)
   ├── protocols:   my_cc.so  jsonrpc_cc.so  jsonrpc_rt.so     (session protocols)
   └── features:    Rating.so  RatingDuckDB.so  CDRMediator.so
                    CallControl.so  monit.so  reapi.so
```

---

## 0. The glue: `globals.h` and `mod.h`

Every `.c` file starts with `#include "../../misc/globals.h"`. That single include is the
umbrella — it `extern`s all shared state and transitively pulls in `main_cfg.h`,
`rt_log.h`, and `mod.h`.

- [src/misc/globals.h](src/misc/globals.h) — externs shared globals: `config` (the
  `re5_server` struct: thread handles, mutexes `sync_bt_thread`/`sync_ccserver_threads`/
  `sync_crating_threads`, log fd `config.log`, pid), `mcfg` (parsed main config),
  runtime flags (`loop_flag`, `call_control_flag`, `rating_flag`, …), and rating params
  (`k_limit_min`, `call_maxsec_limit`, `sim_calls`, `log_debug_level`). Also defines the
  return codes: **`RE_SUCCESS` (0)**, **`RE_ERROR` (1)**, **`RE_ERROR_N` (-1)**.
- [src/mod/mod.h](src/mod/mod.h) — the plugin abstraction. `mod_t` holds a module's name,
  version, `init`/`destroy` fptrs, dependency list, and dlopen `handle`. Lookup via
  `mod_find_module("<name>.so")` + `mod_find_func(handle, "<fn>")`.

**Cross-module bind pattern (memorize this):**
```c
mod_t *m = mod_find_module("Rating.so");
int (*bind)(rt_funcs_t *) = mod_find_func(m->handle, "rt_bind_api");
bind(&rt_api);          /* fills a local function-pointer table */
rt_api.exec(dbp, rtp, 'a');
```
Every pluggable boundary uses this: `net_funcs_t`, `db_sql_t`/`db_nosql_t`, `rt_funcs_t`,
`cdr_funcs_t`, `cc_funcs_t`.

---

## 1. `mem/` — memory  ([src/mem/mem.c](src/mem/mem.c), [src/mem/mem.h](src/mem/mem.h))

| Call | Behaviour |
|---|---|
| `mem_alloc(size)` | `calloc(1,size)` — **always zeroed** |
| `mem_alloc_arr(n,size)` | `calloc(n,size)` |
| `mem_free(ptr)` | `free(ptr)` **+ `malloc_trim(0)` on every call** |

**Rules**
- Never call `malloc`/`calloc`/`free` directly in a module. Use `mem_alloc*` / `mem_free`.
- Memory is zeroed on allocation — **do not** `memset` after `mem_alloc`.
- **Legitimate exceptions only:** `mem.c` itself; `log/rt_log.c` (sits below the mem
  layer — logging the allocator would recurse); and freeing memory a libc call owns
  (e.g. `free()` on `realpath()`'s return in `xml_cfg.c`).

**Performance gotcha (matters for hot paths):** `mem_free` runs `malloc_trim(0)` every
call, returning pages to the OS. `mem/readme.txt` measures freeing 1,000,000 objects
individually at **~118 s** vs **~0.06 s** for one array free. → On per-call/per-CDR hot
paths, prefer **pre-allocated pools** (`mem_alloc_arr`) over alloc/free churn.

---

## 2. `proc/` — threads  ([src/proc/proc_thread.c](src/proc/proc_thread.c), [src/proc/proc_thread.h](src/proc/proc_thread.h))

`proc_thread_t` wraps pthread lifecycle: `mode` (`PROC_THREAD_JOINABLE` /
`PROC_THREAD_DETACHED`), optional `stack_size`, `thread_func`, `args`.
`proc_thread_run()` sets attributes, `pthread_create`s, and joins if JOINABLE.

**Rules**
- Spawn threads via `proc_thread_run()`, not raw `pthread_create`.
- **Not abstracted here:** mutexes (use raw `pthread_mutex_t`, as `cc_tbl_lock` /
  `config.sync_*` do) and **thread pools**. A worker pool is the natural thing to *add*
  to `proc/` rather than hand-roll in a feature module.

---

## 3. `net/` — transport  ([src/net/net.c](src/net/net.c), [src/net/net.h](src/net/net.h))

Vtable model: `net_t = { net_conn_t *conn, net_funcs_t *api }`.
`net_proto_bind(np)` dlopens `<proto>.so` and calls `<proto>_bind_api` to fill the
`net_funcs_t` vtable (`open/close/accept/listen/recv/send/status/connect/s_server`).
`net_serial_server()` just delegates to `api->s_server`.

**Rules**
- Do all networking through `net_t` + the vtable — never raw sockets in a module.
- IP version type is **`net_dom_t`** (`ipv4`/`ipv6`/`loc`); the connection field is
  **`conn->domain`** (not `ipv`). `net_listen(net_conn_t *conn)` takes the conn.
- Add a transport = a new module implementing `<proto>_bind_api`. This is the extension
  seam; concurrency (e.g. a parallel/pooled server) belongs at the net level so all
  transports inherit it (`net_parallel_server` is a stub awaiting implementation).
- The `s_server` contract: `external_func(char *buffer)` reads the request from `buffer`,
  processes, and writes the reply back into the same `buffer`.
- Status: **tcp** and **udp** work; **tls** is incomplete; **sctp** is a stub.

---

## 4. `db/` — database  ([src/db/db.c](src/db/db.c), [src/db/db.h](src/db/db.h))

Vtable model like `net`. `db_t = { db_type_t t (sql|nosql), union{db_sql_t* / db_nosql_t*}
u, db_conn_t *conn }`. Each engine module (`db_pgsql`, `db_mysql`, `db_redis`,
`db_mongodb`, `db_duckdb`) exports `db_<engine>_bind_api`, dlopen'd by `db_engine_bind`,
filling the SQL or NoSQL vtable. **One abstraction spans relational and key-value stores.**

**Canonical SQL lifecycle** (from `db_test`):
```c
db_t *dbp = db_init();
dbp->conn = db_conn_init(mcfg->dbtype, mcfg->dbhost, mcfg->dbname,
                         mcfg->dbport, mcfg->dbuser, mcfg->dbpass, timeout);
db_engine_bind(dbp);                 /* dlopen engine + fill vtable */
db_connect(dbp);
db_select(dbp, "select ... ");       /* or db_query/db_insert/db_update */
db_fetch(dbp);
db_sql_result_t *r = (db_sql_result_t *)dbp->conn->result;
for (int i = 0; i < r->rows; i++)
    use(r->cols_list[0].rows_list[i].row, r->cols_list[1].rows_list[i].row);
db_sql_result_free(r); dbp->conn->result = NULL;
db_close(dbp); db_free(dbp);
```
- **SQL result shape:** `db_sql_result_t { int cols; int rows; db_sql_col_name_t
  *cols_list; }`, each column has `col_name` and `rows_list[r].row`.
- **NoSQL:** `db_set/db_get/db_command`, result in `db_nosql_result_t { char *str;
  db_nosql_pair_t *arr; }`.
- **Injection safety — always escape untrusted input:** `db_sql_escape(src,dst,n)`
  (doubles `'` and `\`) or `db_sql_snprintf(dst,n,fmt,...)` where **`%S`** escapes the arg
  and `%s` passes through.
- **Errors:** `DB_OK` (0) and negative `DB_ERR_*` codes; `db_error(ret)` logs them.
- **Threading:** a `db_t`/connection is **not** shareable across threads without care —
  each worker thread should own its own connection.

---

## 5. `config/` — configuration  ([src/config/xml_cfg.c](src/config/xml_cfg.c), [src/config/main_cfg.c](src/config/main_cfg.c))

- **`xml_cfg`** — libxml2 wrapper. `xml_cfg_doc()` / `xml_cfg_root()` open the document;
  `xml_cfg_params_get(root, node)` reads `<param name= value=>` children into a linked
  list of `xml_param_t`; `xml_cfg_child_get()` handles repeated child nodes.
- **`main_cfg`** — parses the top-level `RateEngine7.xml` into `main_cfg_t` (db creds,
  logs, retries, daemon flag), exposed globally as **`mcfg`**.

**Rule/pattern:** each module ships its own `<mod>_cfg.c` that uses the `xml_cfg_*`
helpers to parse its own config section into its own struct (see
[src/mod/CallControl/cc_cfg.c](src/mod/CallControl/cc_cfg.c) as the template). Global
settings come from `mcfg`.

---

## 6. `log/` — logging  ([src/log/rt_log.c](src/log/rt_log.c), [src/log/rt_log.h](src/log/rt_log.h))

Line format: `|timestamp|func|msg|`. Use the macros — never `printf`/`fprintf`:

| Macro | When |
|---|---|
| `LOG(func, msg, ...)` | always (writes to `config.log` fd, or stderr if logging disabled) |
| `DBG(func, msg, ...)` | if `log_debug_level >= LOG_LEVEL_DEBUG` |
| `DBG2(msg, ...)` | if `log_debug_level >= LOG_LEVEL_TIME_DEBUG` (auto-adds `__func__`/`__LINE__`) |

Driven by globals `config.log`, `log_separator`, `log_debug_level`.

---

## 7. `misc/` — startup, utilities, stats

Not one layer but the support belt: process lifecycle, shared globals, and reusable
helpers. **Use these instead of reinventing them.**

**Startup & orchestration**
- [src/misc/init.c](src/misc/init.c) — **the single definition site** for every `extern`
  global declared in `globals.h` (`mcfg`, `config`, `mod_lst`, `loop_flag`, `k_limit_min`,
  `sim_calls`, `log_debug_level`, …). This is the extern-in-header / define-once rule in
  practice. `init_globals()` and `init_log_params()` set them up.
- [src/misc/re7_manager.c](src/misc/re7_manager.c) — orchestration. `re7_starter()` starts
  the enabled subsystems based on flags (`call_control_flag`, `get_cdrs_flag`,
  `rating_flag`); `re7_manager()` is the supervisor loop (log-rotation check + sleep while
  `loop_flag == 't'`).
- [src/misc/daemon.c](src/misc/daemon.c) — `daemonize()` / `signal_handler()` /
  `stop_daemon()` (fork, pidfile, signals).
- `re5_fstat` (file size, for rotation), `reload_logfile`, `chk_db_version` (DB schema
  version guard).

**`misc/exten/` — reusable utilities** (prefer these over ad-hoc code):
- [time_funcs](src/misc/exten/time_funcs.h) — `convert_ts_to_epoch` / `convert_epoch_to_ts`
  / `get_weekday_from_epoch` / `check_date_valid` / `current_datetime`, and the **`RTIMER`
  macros** for microsecond timing (use these when measuring hot-path latency).
- [str_ext](src/misc/exten/str_ext.h) — string helpers; return `mem_alloc`'d buffers
  (caller `mem_free`s).
- [file_list](src/misc/exten/file_list.h) — directory listing (`file_list_get`); used e.g.
  to enumerate per-interface config files under `cc_int/`.
- [json_ext](src/misc/exten/json_ext.h) — JSON framework over `json-c`: a declarative
  `json_ext_obj_t` schema (type + name + value) with put/get/create. Build and parse JSON
  through this, not raw `json-c`.
- [json_rpc](src/misc/exten/json_rpc.h) — JSON-RPC 2.0 framework on top of `json_ext`
  (request/response/error, method→param/result schemas via `jsonrpc_proto_type_t`,
  transactions). Used by `jsonrpc_cc` / `jsonrpc_rt`.

**`misc/stat/` — cross-process telemetry** ([src/misc/stat/stat.c](src/misc/stat/stat.c)):
SysV shared memory (key `STAT_SHMEM_KEY 5767`) holding a `stat_data_t` — live sim-call
count, rating/CDRMediator counters, per-process flags, manager pid. `stat_init` (create +
zero), `stat_read` (attach), `stat_write`, `stat_remove`. Producers publish (the
CallControl janitor writes the sim count); monitors (`monit`/`reapi`) read it. This is the
channel for live metrics — e.g. the concurrent-call count a shared-pcard split needs.

---

## Module author checklist

1. `#include "../../misc/globals.h"` first (brings config/mcfg/flags/`RE_*` + log/config/mod).
2. **Memory:** `mem_alloc` / `mem_alloc_arr` / `mem_free` only; zeroed on alloc; pool hot paths.
3. **Threads:** `proc_thread_run`.
4. **Network:** `net_t` + vtable + `net_proto_bind`; IP version is `net_dom_t`/`conn->domain`.
5. **Database:** `db_t` + vtable; own connection per thread; escape input (`db_sql_snprintf` `%S`).
6. **Config:** own `<mod>_cfg` via `xml_cfg_*`; globals from `mcfg`.
7. **Logging:** `LOG()` / `DBG()`.
8. **Cross-module:** bind function tables via `mod_find_func("<mod>_bind_api")`.
9. **Return codes:** `RE_SUCCESS` (0) / `RE_ERROR` (1) / `RE_ERROR_N` (-1).

## Build & known gotchas

- Build: `make RE7Core` (core lib), then `make module name=<Module>` per module. Modules
  link `-lre7core`.
- **Header-defined globals need `extern`** (modern gcc defaults to `-fno-common`): declare
  a shared global as `extern` in the header and define it once in a single `.c`. A bare
  `T x;` in a header included by multiple `.c` files causes *multiple definition* link
  errors. Core globals follow this: `extern` in `globals.h`, defined once in
  [src/misc/init.c](src/misc/init.c). (`rating.h` also does this correctly.)
- **System-header includes:** `globals.h` uses `pthread_t`/`time_t`/`pid_t` in its structs.
  Whether those (`<pthread.h>`, `<time.h>`, `<sys/types.h>`) and `<stdlib.h>` are pulled in
  centrally (in `globals.h`) or per-file is a project convention decision — keep it
  consistent, don't sprinkle ad-hoc includes.
- **Legitimate raw libc:** `atoi` (config parsing), `realpath`+`free` (xml_cfg), and the
  logger's own `malloc`/`free` are the only sanctioned uses of raw libc alloc/parse.
