# RateEngine V7 — working notes for Claude

Telecom billing/rating engine in C. V7 is a **modular plugin architecture**: a core
library (`libre7core`) plus `.so` modules loaded at runtime via `dlopen`, wired together
through **function-pointer tables** ("bind APIs"). Author: Dimitar Kokov.

## Read first
**[doc/core_architecture.md](doc/core_architecture.md)** — the orientation doc. It covers
the core layers (`mem`, `proc`, `net`, `db`, `config`, `log`, `misc`), the plugin/bind
conventions, and the build gotchas. Start there before touching any module.

## Hard rules (do not violate)
- **Never use raw libc where a core layer exists.** Use the abstractions:
  - Memory → `mem_alloc` / `mem_alloc_arr` / `mem_free` (zeroed on alloc; never `malloc`/`free`/`memset`-after-alloc).
  - Threads → `proc_thread_run` (not raw `pthread_create`).
  - Network → `net_t` + vtable + `net_proto_bind` (not raw sockets).
  - Database → `db_t` + vtable; escape input with `db_sql_snprintf` (`%S`).
  - Config → `xml_cfg_*` helpers + global `mcfg`; each module has its own `<mod>_cfg.c`.
  - Logging → `LOG()` / `DBG()` macros (never `printf`/`fprintf`).
  - Time/string/JSON/dir → use `misc/exten/*` (`time_funcs`, `str_ext`, `json_ext`, `json_rpc`, `file_list`).
- **Cross-module calls** go through `mod_find_func("<mod>_bind_api")` + a function-pointer table.
- **Return codes:** `RE_SUCCESS` (0) / `RE_ERROR` (1) / `RE_ERROR_N` (-1).
- **Shared globals:** `extern` in a header, defined once in a `.c` (modern gcc is
  `-fno-common`). Core globals live in `globals.h` (extern) + [src/misc/init.c](src/misc/init.c) (defined).
- **Don't sprinkle ad-hoc system includes.** If a baseline libc header is missing, decide
  on a *centralized* fix (usually `globals.h`), not per-file additions.

## Build
- Core lib: `make RE7Core`  →  `libre7core.so`
- A module: `make module name=<Module>`  (links `-lre7core`)
- Run from `src/`. Include flags come from `src/config.mk`.

## Workflow expectations
- **Prefer analysis and small, reviewable changes.** Confirm approach before large,
  cross-file, or billing-logic edits — this is a billing engine; correctness = money.
- Don't commit or push unless asked.
