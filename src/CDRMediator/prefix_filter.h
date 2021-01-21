#ifndef PREFIX_FILTER_H
#define PREFIX_FILTER_H

#define FILTERS 10

typedef struct filter
{
    int id;
    char filtering_prefix[32];
    int filtering_number;
    char replace_str[32];
    int cdr_server_id;
    int len;
}filter;

void prefix_filter_cuti_replace(char *prefix,char *cut_string,int i,char *replace_string,char *ret,int prefix_len);
filter *prefix_filter_init(int num);
filter *prefix_filter_get(PGconn *conn,int cdr_profile_id);

#endif
