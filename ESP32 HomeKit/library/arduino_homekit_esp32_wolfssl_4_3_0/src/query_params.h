#ifndef __HOMEKIT_QUERY_PARAMS__
#define __HOMEKIT_QUERY_PARAMS__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _query_param {
    char *name;
    char *value;

    struct _query_param *next;
} query_param_t;

query_param_t *query_params_parse(const char *s);
query_param_t *query_params_find(query_param_t *params, const char *name);
void query_params_free(query_param_t *params);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_QUERY_PARAMS__
