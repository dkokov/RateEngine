/*
 * cdr_ingest_safety.c — regression checks for the CDRMediator ingestion path.
 *
 * Self-contained (no DB, no project libs), in the style of tariff_pricing.c:
 * each function under test is mirrored here by a VERBATIM copy of the source,
 * kept in sync with these origins:
 *   prefix_filter_cuti_replace  <- src/mod/CDRMediator/prefix_filter.c
 *   cdr_field_set               <- src/mod/CDRMediator/cdr.c   (Phase 1.2)
 *   db_sql_escape               <- src/db/db.c                 (Phase 1.1)
 *
 * It pins three behaviours:
 *   1) prefix cut/replace semantics (match, cut, replace, prefix_len guard)
 *   2) bounded CDR field copy truncates instead of overflowing  (Phase 1.2)
 *   3) SQL escaping doubles quotes/backslashes + honors the size guard (1.1)
 *
 * Build & run:
 *   cc -O2 -Wall src/unit_tests/cdr_ingest_safety.c -o /tmp/cdr_ingest_safety \
 *     && /tmp/cdr_ingest_safety
 * Exit 0 = all pass, 1 = a regression.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* verbatim: prefix_filter_cuti_replace (src/mod/CDRMediator/prefix_filter.c) */
static void prefix_filter_cuti_replace(char *prefix,char *cut_string,int i,char *replace_string,char *ret,int prefix_len)
{
    int len,p,c;
    char buf[32];
    char replace_str[80];

    bzero(replace_str,80);

    len = 0;

    len = strlen(cut_string);

    strncpy(buf,prefix,len);

    buf[len] = '\0';

    if(prefix_len > 0)
    {
		if(strlen(prefix) == prefix_len) goto cont;
		else goto end_rep;
	}

    cont:
    if(!strcmp(buf,cut_string))
    {
		c=0;
		for(p=i;p<(strlen(prefix));p++)
		{
			ret[c] = prefix[p];
			c++;
		}
		ret[c]='\0';

		if(strcmp(replace_string,""))
		{
			strcpy(replace_str,replace_string);
			strcat(replace_str,ret);
			strcpy(ret,replace_str);
		}
    }
    else
    {
		end_rep:
		ret[0]='\0';
    }
}

/* ------------------------------------------------------------------ */
/* verbatim: cdr_field_set (src/mod/CDRMediator/cdr.c, Phase 1.2) */
static void cdr_field_set(char *dst,const char *src,size_t dst_size)
{
	if((dst == NULL)||(src == NULL)||(dst_size == 0)) return;

	strncpy(dst,src,dst_size - 1);
	dst[dst_size - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* verbatim: db_sql_escape (src/db/db.c, Phase 1.1) */
static int db_sql_escape(const char *src,char *dst,int dst_size)
{
	int i,j;

	if(src == NULL || dst == NULL || dst_size <= 0) return -1;

	j = 0;
	for(i = 0; src[i] != '\0'; i++) {
		if(j >= (dst_size - 2)) return -2;

		if(src[i] == '\'') {
			dst[j++] = '\'';
			dst[j++] = '\'';
		} else if(src[i] == '\\') {
			dst[j++] = '\\';
			dst[j++] = '\\';
		} else {
			dst[j++] = src[i];
		}
	}

	dst[j] = '\0';

	return j;
}

/* ------------------------------------------------------------------ */
static int failures = 0;

static void check_str(const char *name,const char *got,const char *want)
{
	if(strcmp(got,want) == 0) {
		printf("  PASS %s\n",name);
	} else {
		printf("  FAIL %s: got \"%s\" want \"%s\"\n",name,got,want);
		failures++;
	}
}

static void check_int(const char *name,long got,long want)
{
	if(got == want) {
		printf("  PASS %s\n",name);
	} else {
		printf("  FAIL %s: got %ld want %ld\n",name,got,want);
		failures++;
	}
}

static void check_true(const char *name,int cond)
{
	if(cond) printf("  PASS %s\n",name);
	else { printf("  FAIL %s\n",name); failures++; }
}

/* ------------------------------------------------------------------ */
static void test_prefix_filter(void)
{
	char ret[80];

	printf("== prefix_filter_cuti_replace ==\n");

	/* match, cut first 5 digits, prepend replacement */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("0035912345","00359",5,"359",ret,0);
	check_str("match+cut+replace",ret,"35912345");

	/* match, cut first 5 digits, no replacement */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("0035912345","00359",5,"",ret,0);
	check_str("match+cut, no replace",ret,"12345");

	/* no prefix match -> empty result */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("004912345","00359",5,"",ret,0);
	check_str("no match -> empty",ret,"");

	/* prefix_len guard: length mismatch suppresses replacement */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("0035912345","00359",5,"",ret,11);
	check_str("prefix_len mismatch -> empty",ret,"");

	/* prefix_len guard: exact length allows replacement */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("0035912345","00359",5,"",ret,10);
	check_str("prefix_len match -> cut",ret,"12345");

	/* cutting the whole number leaves nothing */
	bzero(ret,sizeof(ret));
	prefix_filter_cuti_replace("00359","00359",5,"",ret,0);
	check_str("cut entire number -> empty",ret,"");
}

static void test_field_bounding(void)
{
	/* field followed by a canary; an unbounded copy would smash the canary */
	struct { char field[80]; char canary[8]; } s;

	/* heap source of runtime length: keeps the deliberate-truncation input
	 * opaque to the optimizer (no -Wstringop-truncation on the verbatim copy). */
	int n = 200;
	char *longsrc = malloc(n + 1);

	printf("== cdr_field_set (overflow regression) ==\n");

	memset(&s,0,sizeof(s));
	strcpy(s.canary,"GUARD");

	memset(longsrc,'A',n);
	longsrc[n] = '\0';

	cdr_field_set(s.field,longsrc,sizeof(s.field));
	free(longsrc);

	check_int("truncated to field size-1",(long)strlen(s.field),79);
	check_true("NUL terminated",s.field[79] == '\0');
	check_str("canary intact (no overflow)",s.canary,"GUARD");

	/* NULL src must be a no-op, not a crash */
	strcpy(s.field,"keep");
	cdr_field_set(s.field,NULL,sizeof(s.field));
	check_str("NULL src is a no-op",s.field,"keep");

	/* short value copies verbatim */
	cdr_field_set(s.field,"359888",sizeof(s.field));
	check_str("short value copied",s.field,"359888");
}

static void test_sql_escape(void)
{
	char out[256];
	int rc;

	printf("== db_sql_escape (injection regression) ==\n");

	/* the classic break-out attempt: the lone quote must be doubled */
	rc = db_sql_escape("x'; drop table cdrs;--",out,sizeof(out));
	check_true("escape returns length",rc > 0);
	check_str("single quote doubled",out,"x''; drop table cdrs;--");

	/* backslash doubled */
	rc = db_sql_escape("a\\b",out,sizeof(out));
	check_str("backslash doubled",out,"a\\\\b");
	check_true("backslash rc",rc == 4);

	/* benign input unchanged */
	rc = db_sql_escape("+35912345",out,sizeof(out));
	check_str("benign unchanged",out,"+35912345");

	/* size guard: too small a buffer is reported, not overflowed */
	rc = db_sql_escape("''''''''''",out,4);
	check_true("size guard returns -2",rc == -2);
}

int main(void)
{
	test_prefix_filter();
	test_field_bounding();
	test_sql_escape();

	printf("\n%s (%d failure%s)\n",failures ? "FAILED" : "OK",failures,failures == 1 ? "" : "s");

	return failures ? 1 : 0;
}
