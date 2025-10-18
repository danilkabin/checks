#include "core/opium_core.h"

/* Initialize configuration structure */
opium_int_t
opium_conf_init(opium_conf_t *conf, opium_log_t *log)
{
    if (!conf || !log) {
        return OPIUM_RET_ERR;
    }

    memset(conf, 0, sizeof(opium_conf_t));
   
    opium_int_t result;

    result = opium_arena_init(&conf->arena, log);
    if (result != OPIUM_RET_OK) {
        opium_log_err(log, "Failed to initialize configuration arena");
        return OPIUM_RET_ERR;
    }

    return OPIUM_RET_OK;
}

/* Cleanup configuration structure */
void
opium_conf_exit(opium_conf_t *conf)
{
    if (!conf) {
        return;
    }

    opium_array_destroy(&conf->workers);
    opium_arena_exit(&conf->arena);
    memset(conf, 0, sizeof(opium_conf_t));
}

/* Set string configuration value */
char *
opium_conf_set_str(opium_conf_t *conf, opium_command_t *cmd, void *data)
{
    if (!conf || !cmd || !data) {
        return "Invalid input parameters";
    }

    u_char *src = (u_char *)data;
    opium_string_t *field = (opium_string_t *)((char *)cmd->conf + cmd->offset);
    size_t len = opium_strlen(src);

    field->data = opium_arena_alloc(&conf->arena, len + 1);
    if (!field->data) {
        return "Memory allocation failed for string";
    }

    opium_memcpy(field->data, src, len);
    field->data[len] = '\0';
    field->len = len;

    return NULL;
}

/* Set numeric configuration value */
char *
opium_conf_set_num(opium_conf_t *conf, opium_command_t *cmd, void *data)
{
    if (!conf || !cmd || !data) {
        return "Invalid input parameters";
    }

    opium_uint_t *field = (opium_uint_t *)((char *)cmd->conf + cmd->offset);
    *field = *(opium_uint_t *)data;
    return NULL;
}

/* Set flag configuration value */
char *
opium_conf_set_flag(opium_conf_t *conf, opium_command_t *cmd, void *data)
{
    if (!conf || !cmd || !data) {
        return "Invalid input parameters";
    }

    opium_flag_t *field = (opium_flag_t *)((char *)cmd->conf + cmd->offset);
    *field = *(opium_flag_t *)data;
    return NULL;
}

char *
opium_conf_validate(opium_conf_t *conf)
{
    if (!conf) {
        return "Invalid configuration pointer";
    }

    if (conf->server_conf.port == 0) {
        return "Server port not specified";
    }

    if (conf->server_conf.ip.len == 0) {
        return "Server IP not specified";
    }

    if (conf->workers.nelts == 0) {
        return "No worker processes configured";
    }

    return NULL;
}
