/*
 * Copyright 2025 steadybit GmbH. All rights reserved.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_http_complex_value_t  *sleep_ms;
} ngx_http_sleep_loc_conf_t;

static ngx_int_t ngx_http_sleep_init(ngx_conf_t *cf);
static void *ngx_http_sleep_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_sleep_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_sleep_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_sleep_handler(ngx_http_request_t *r);

static ngx_command_t ngx_http_sleep_commands[] = {
    { ngx_string("sleep_ms"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_sleep_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_sleep_loc_conf_t, sleep_ms),
      NULL },
    ngx_null_command
};

static ngx_http_module_t ngx_http_sleep_module_ctx = {
    NULL,                          /* preconfiguration */
    ngx_http_sleep_init,           /* postconfiguration */
    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */
    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
    ngx_http_sleep_create_loc_conf,/* create location configuration */
    ngx_http_sleep_merge_loc_conf  /* merge location configuration */
};

ngx_module_t ngx_http_sleep_module = {
    NGX_MODULE_V1,
    &ngx_http_sleep_module_ctx,    /* module context */
    ngx_http_sleep_commands,       /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static void *
ngx_http_sleep_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_sleep_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_sleep_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->sleep_ms = NULL;

    return conf;
}

static char *
ngx_http_sleep_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_sleep_loc_conf_t *prev = parent;
    ngx_http_sleep_loc_conf_t *conf = child;

    if (conf->sleep_ms == NULL) {
        conf->sleep_ms = prev->sleep_ms;
    }

    return NGX_CONF_OK;
}

static char *
ngx_http_sleep_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_sleep_loc_conf_t *slcf = conf;
    ngx_str_t *value;
    ngx_http_compile_complex_value_t ccv;

    if (slcf->sleep_ms != NULL) {
        return "is duplicate";
    }

    value = cf->args->elts;

    slcf->sleep_ms = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
    if (slcf->sleep_ms == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = slcf->sleep_ms;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_sleep_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_sleep_handler;

    return NGX_OK;
}

static ngx_int_t
ngx_http_sleep_handler(ngx_http_request_t *r)
{
    ngx_http_sleep_loc_conf_t  *slcf;
    ngx_str_t                   val;
    ngx_int_t                   sleep_time;

    slcf = ngx_http_get_module_loc_conf(r, ngx_http_sleep_module);

    if (slcf->sleep_ms == NULL) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, slcf->sleep_ms, &val) != NGX_OK) {
        return NGX_ERROR;
    }

    if (val.len == 0) {
        return NGX_DECLINED;
    }

    sleep_time = ngx_atoi(val.data, val.len);
    if (sleep_time == NGX_ERROR || sleep_time < 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid sleep_ms value \"%V\"", &val);
        return NGX_DECLINED;
    }

    if (sleep_time == 0) {
        return NGX_DECLINED;
    }

    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                  "sleeping for %i ms", sleep_time);

    ngx_msleep(sleep_time);

    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                  "finished sleeping for %i ms", sleep_time);

    return NGX_DECLINED;
}
