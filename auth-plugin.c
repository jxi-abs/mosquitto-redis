#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <mosquitto.h>
#include <mosquitto_plugin.h>

#include <hiredis/hiredis.h>

// -- function return code of MQTT
// MOSQ_ERR_CONN_PENDING = -1,
// MOSQ_ERR_SUCCESS = 0,
// MOSQ_ERR_NOMEM = 1,
// MOSQ_ERR_PROTOCOL = 2,
// MOSQ_ERR_INVAL = 3,
// MOSQ_ERR_NO_CONN = 4,
// MOSQ_ERR_CONN_REFUSED = 5,
// MOSQ_ERR_NOT_FOUND = 6,
// MOSQ_ERR_CONN_LOST = 7,
// MOSQ_ERR_TLS = 8,
// MOSQ_ERR_PAYLOAD_SIZE = 9,
// MOSQ_ERR_NOT_SUPPORTED = 10,
// MOSQ_ERR_AUTH = 11,
// MOSQ_ERR_ACL_DENIED = 12,
// MOSQ_ERR_UNKNOWN = 13,
// MOSQ_ERR_ERRNO = 14,
// MOSQ_ERR_EAI = 15,
// MOSQ_ERR_PROXY = 16


typedef struct udata_t
{
    char host[128];
    int port;
} udata;

void log_debug(char * format, ...) {
    va_list aptr;
    char buf[1024];
    va_start(aptr, format);

    vsprintf(buf, format, aptr);
    mosquitto_log_printf(MOSQ_LOG_DEBUG, buf);
}

char buf[10240];

int mosquitto_auth_plugin_version(void)
{
    log_debug("API call : mosquitto_auth_plugin_version(void)");
    return MOSQ_AUTH_PLUGIN_VERSION;
}

int mosquitto_auth_plugin_init(void **user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count)
{
    log_debug("API call : mosquitto_auth_plugin_init");

    udata *udata = *user_data = calloc(1, sizeof(struct udata_t));
    udata->port = 6379;
    strcpy(udata->host, "localhost");

    for (int i = 0; i < auth_opt_count; ++i) {
        log_debug("   plugin init, auth_opts[%d], %s : %s", i, auth_opts[i].key, auth_opts[i].value);
        if (strcmp("auth_opt_redis_host", auth_opts[i].key) == 0) {
            strncpy(udata->host, auth_opts[i].value, 127);
            log_debug("udata->host : %s", udata->host);
        }
        if (strcmp("auth_opt_redis_port", auth_opts[i].key) == 0) {
            udata->port = atoi(auth_opts[i].value);
            log_debug("udata->host : %d", udata->port);
        }
    }

    log_debug("Loaded mosquitto redis plugin");
    return MOSQ_ERR_SUCCESS;

    (void)udata;
}

int mosquitto_auth_plugin_cleanup(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count)
{

    log_debug("API call : mosquitto_auth_plugin_cleanup");

    udata *udata = user_data;
    free(udata);
    return MOSQ_ERR_SUCCESS;

    (void) auth_opts;
    (void) auth_opt_count;
}

int mosquitto_auth_security_init(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
    udata *udata = user_data;

    log_debug("API call : mosquitto_auth_security_init");

    for (int i = 0; i < auth_opt_count; ++i) {
        log_debug("API security init, auth_opts[%d], %s : %s", i, auth_opts[i].key, auth_opts[i].value);
    }

    // return MOSQ_ERR_UNKNOWN;
    return MOSQ_ERR_SUCCESS;

    (void) udata;
    (void) reload;
}

int mosquitto_auth_security_cleanup(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
    udata *udata = user_data;
    log_debug("API call : mosquitto_auth_security_cleanup");
    return MOSQ_ERR_SUCCESS;

    (void) udata;
    (void) auth_opts;
    (void) auth_opt_count;
    (void) reload;
}

int mosquitto_auth_acl_check(void *user_data, const char *clientid, const char *username, const char *topic, int access)
{
    log_debug("API call : mosquitto_auth_acl_check");
    log_debug("API clientid %s, username %s, topic %s, access %d", clientid, username, topic, access);

    udata *udata = user_data;

    char * session_id = NULL;
    {
        char * buf = strdup(topic);
        char * s = buf;
        char * token = NULL;
        int i = 0;
        while ((token = strsep(&s, "/"))) {
            log_debug("i %d, token %s", i, token);
            if (i==0 && strcmp(token, "chat") != 0) {
                log_debug("not a valid chat topic");
                break;
            }
            if (i==2) {
                session_id = strdup(token);
                log_debug("store session id for future check : %s", session_id);
            }
            ++i;
        }
        free(buf);
    }

    int verified = 0;
    if (session_id != NULL) {
        log_debug("check redis %s:%d, for session id : %s", udata->host, udata->port, session_id);

        // redisContext *c = redisConnect("localhost", port);
        redisContext *c = redisConnect(udata->host, udata->port);

        if (c != NULL && c->err) {
            log_debug("Error: %s", c->errstr);
            // handle error
        } else {
            log_debug("Connected to Redis");

            // cmd example :
            //"AUTH password"

            char cmd[128];
            sprintf(cmd, "GET %s", "3e42721d-9525-4ddf-bd3c-cb5e6caefdda");
            // sprintf(cmd, "GET %s", session_id);
            redisReply * r = redisCommand(c, cmd);
            log_debug("redis reply type : %d", r->type);
            if (r->type == REDIS_REPLY_STRING) {
                // pwhash = strdup(r->str);
                verified = 1;
                log_debug("verified, redis reply string : %s", r->str);
            }
            freeReplyObject(r);
        }

        redisFree(c);

        free(session_id);
    }
    else {
        log_debug("other than chat, pass them through");
        verified = 1;
    }

    if (verified == 1)
        return MOSQ_ERR_SUCCESS;
    else
        return MOSQ_ERR_AUTH;

    (void) user_data;
    (void) clientid;
    (void) username;
    (void) topic;
    (void) access;
}

int mosquitto_auth_unpwd_check(void *user_data, const char *username, const char *password)
{
    udata *udata = user_data;

    log_debug("API call : mosquitto_auth_unpwd_check");
    log_debug("API username %s, topic %s", username, password);

    return MOSQ_ERR_SUCCESS;
    (void) udata;
}

int mosquitto_auth_psk_key_get(void *user_data, const char *hint, const char *identity, char *key, int max_key_len)
{
    log_debug("API call : mosquitto_auth_psk_key_get");
    log_debug("API hint %s, indentity %s, key %s, max_key_len %d", hint, identity, key, max_key_len);

    return 1;

    (void) user_data;
    (void) hint;
    (void) identity;
    (void) key;
    (void) max_key_len;
}

