/*  THIS .CONF FILE PARSER HANDLES THESE TYPES OF FAILURES:

        INVALID PARAM/LINE   OK      fail, not immediate
        INVALID VALUE        OK      fail, not immediate
        2 LONG  VALUE        OK      fail, not immediate
        REPEATED PARAM       OK      fail, not immediate
        MISSING PARAM        OK      __check_missing_params()

        2 LONG LINE          OK      fail completely, why immediately???

    Features:
        * Empty/commented lines
        * Trailing comments/whitespace
        * Ignores  leading  whitespace

    Limitations:
        *   Only int and str types allowed
        *   star* chars always interpreted as comments
        *   quote chars not allowed for str types

*/

#include <string.h>     // strstr, strcmp, strlen
#include <stdio.h>      // FILE, printf
#include <stdlib.h>     // exit, malloc, strtol
#include <limits.h>     // INT_MAX

#include "config.h"     // struct L8Configuration

#define CONFIG_PATH         "/etc/l8/l8.conf"
#define MAX_LINE_LEN        80
#define PARAM_INIT_VALUE    -1

#define MAX_CONFIG_PARAMS   10    // edit when adding parameters
const char * CONFIG_PARAMS[MAX_CONFIG_PARAMS] = {
    "client_name",
    "server_name",
    "server_ip",
    "server_port",
    "local_bind_ip",
    "local_bind_port",
    "client_recv_delay",
    "server_frwd_delay",
    "server_recv_delay",
    "client_frwd_delay",
};

int __is_whitespace(char * c)
{
    if (*c == ' ' || *c == '\t') return 1;
    return 0;
}

int __is_comment(char * c)
{
    if (*c == '#') return 1;
    // if (*c == '/' && *(c+1) == '/') return 1;
    return 0;
}

void __print_until_whitespace(char * line_char)
{
    while ( !__is_whitespace(line_char) && *line_char != '\0' ) {
        printf("%c", *line_char);
        line_char++;
    }
}

int __check_end_of_line(char * eol)
{
    // CHECKS IF EOL IS SOMETHING OTHER THAN WHITESPACE/COMMENT/EOL
    while ( __is_whitespace(eol) ) eol ++;
    if ( __is_comment(eol) ) return 1;
    if ( *eol == '\0' ) return 1;
    return 0;
}


int __set_number(const char * param_name, char * value, int line_num, long * param)
{
    // CONVERTS STR TO NUMBER
    char * p;
    long number = strtol( value, &p, 10 );
    if ( *p != '\0' || number > INT_MAX ) {
        printf("L8 config: line %02d [%s] value [%s] is not a valid integer\n", line_num, param_name, value);
        return line_num;  // NOT an int or too large
    }

    // CHECKS REPEATED ASSIGNMENT
    if ( *param != PARAM_INIT_VALUE ) {
        printf("L8 config: line %02d [%s] repeated assignment [%04ld]\n", line_num, param_name, number);
        return line_num;
    }

    *param = number;
    return 0;
}

int __set_string(const char * param_name, char * value, int line_num, char * param)
{
    // CHECKS REPEATED ASSIGNMENT
    if ( *param != '\0' ) {
        printf("L8 config: line %02d [%s] repeated assignment [%s]\n", line_num, param_name, value);
        return line_num;
    }

    // string length has already been validated @ __trim_param_value
    strcpy(param, value);
    return 0;
}

int __eval_param_value(const char * param, char * value, int line_num, struct L8Configuration * conf)
{
    if (strcmp(param, "client_name") == 0) return __set_string(param, value, line_num, conf->client_name);
    if (strcmp(param, "server_name") == 0) return __set_string(param, value, line_num, conf->server_name);

    // use index of CONFIG_PARAMS instead of manually checking ???
    if (strcmp(param, "server_ip") == 0) return __set_string(param, value, line_num, conf->server_ip);
    if (strcmp(param, "server_port") == 0) return __set_number(param, value, line_num, &conf->server_port);

    if (strcmp(param, "local_bind_ip") == 0) return __set_string(param, value, line_num, conf->local_bind_ip);
    if (strcmp(param, "local_bind_port") == 0) return __set_number(param, value, line_num, &conf->local_bind_port);

    if (strcmp(param, "client_recv_delay") == 0) return __set_number(param, value, line_num, &conf->client_recv_delay);
    if (strcmp(param, "server_frwd_delay") == 0) return __set_number(param, value, line_num, &conf->server_frwd_delay);
    if (strcmp(param, "server_recv_delay") == 0) return __set_number(param, value, line_num, &conf->server_recv_delay);
    if (strcmp(param, "client_frwd_delay") == 0) return __set_number(param, value, line_num, &conf->client_frwd_delay);

    // WTF ???
    return -1;

}

int __trim_param_value(const char * param, char * value_line, int line_offset, int line_num, struct L8Configuration * conf)
{
    // ONLY ALLOWED PARAMETERS GET HERE...
    // TRIMS LEADING AND TRAILING WHITESPACE FROM VALUE_LINE

    // ignore leading whitespace from value_line
    for (int x = 0; x < MAX_LINE_LEN - strlen(param); x++) {
        if ( ! __is_whitespace(value_line) ) break;
        value_line++;
        // server_port    8090    # comment
        //             ^ this offset
    }


    // trim trailing whitespace/comments from value
    char value[MAX_CFG_VALUE_LEN];
    memset(value, '\0', MAX_CFG_VALUE_LEN*sizeof(char));  // why keeps param value of previous function_call() ??
    for (int y = 0; y < 1000; y++) {
        value[y] = *value_line;
        value_line++;
        if ( __is_whitespace(value_line) || *value_line == '\0' ) break;
        if (y == MAX_CFG_VALUE_LEN - 1) {
            printf("L8 config: line %02d [%s] value [%s%c] exceeds maximum length [%d]\n", line_num, param, value, *value_line, MAX_CFG_VALUE_LEN);
            return line_num;
        }
    }

    // validate EOL
    int valid_eol = __check_end_of_line(value_line);
    if (!valid_eol) {
        printf("L8 config: line %02d [%s] failed to parse EOL\n", line_num, value_line);
        return line_num;
    }

    return __eval_param_value(param, value, line_num, conf);

}


int __eval_line_param(char * line, int line_num, struct L8Configuration * conf)
{

    // TRY TO FIND EACH CONFIG_PARAMS ITEM AT START-OF-LINE
    for (int x = 0; x< MAX_CONFIG_PARAMS; x++) {

        const char * check_param  = CONFIG_PARAMS[x];
        char * found_param = strstr(line, check_param);

        // FOUND CHECKED PARAM AT START-OF-LINE
        if (found_param != NULL && found_param == line) {

            size_t line_offset  = strlen(check_param);
            char * trimmed_line = line + line_offset;
            // server_port    8090    # comment
            //         ^ offset

            // whitespace ensures param is exactly matched
            if ( __is_whitespace( trimmed_line ) ) {
                return __trim_param_value(check_param, trimmed_line, line_offset, line_num, conf);
            }

        }

    }

    // INVALID PARAM/LINE
    printf("L8 config: line %02d [", line_num);
    __print_until_whitespace(line);
    printf("] parameter is not valid\n");

    return line_num;
}

int __parse_config_line(char * line, int line_len, int line_num, struct L8Configuration * conf)
{
    // RETURNS LINE_NUM IF INVALID LINE

    for (int line_offset=0; line_offset < line_len; line_offset++) {
        char * c = line + line_offset;
        if ( __is_comment(c)    ) break;
        if ( __is_whitespace(c) ) continue;
        return __eval_line_param(c, line_num, conf);
    }

    // LINE IS A COMMENT OR WHITESPACE ONLY
    return 0;
}


int __parse_config_file(FILE * config_file, struct L8Configuration * conf)
{
    // RETURNS NUMBER OF INVALID LINES IN CONFIG FILE
    int invalid_lines = 0;

    int line_num = 1;
    int line_len = 0;
    char line[MAX_LINE_LEN];
    memset(line, '\0', MAX_LINE_LEN*sizeof(char));

    while ( 1 ) {

        int c = fgetc(config_file);

        if ( c == EOF ) {
            if ( __parse_config_line(line, line_len, line_num, conf) ) invalid_lines++;
            break;
        }

        if ( c == '\n' ) {
            if ( __parse_config_line(line, line_len, line_num, conf) ) invalid_lines++;
            memset(line, '\0', MAX_LINE_LEN*sizeof(char));
            line_num++;
            line_len = 0;

        } else {
            line[line_len] = c;
            line_len++;

        }

        // if ( line_len >= MAX_LINE_LEN ) {
        //     printf("L8 config: line %02d exceeds maximum length [%d].\n", line_num, MAX_LINE_LEN);
        //     invalid_lines++;
        // }

    }

    return invalid_lines;

}


struct L8Configuration * config_init()
{
    struct L8Configuration * conf = malloc(sizeof(struct L8Configuration));
    memset(conf->client_name, '\0', MAX_CFG_VALUE_LEN * sizeof(char));
    memset(conf->server_name, '\0', MAX_CFG_VALUE_LEN * sizeof(char));
    memset(conf->server_ip, '\0', MAX_CFG_VALUE_LEN * sizeof(char));
    memset(conf->local_bind_ip, '\0', MAX_CFG_VALUE_LEN * sizeof(char));
    conf->server_port = PARAM_INIT_VALUE;
    conf->local_bind_port = PARAM_INIT_VALUE;
    conf->client_recv_delay = PARAM_INIT_VALUE;
    conf->server_frwd_delay = PARAM_INIT_VALUE;
    conf->server_recv_delay = PARAM_INIT_VALUE;
    conf->client_frwd_delay = PARAM_INIT_VALUE;
    return conf;
}

int __check_missing_params(struct L8Configuration * conf)
{
    // MISSING PARAMS
    int missing_params = 0;
    if (conf->client_name[0] == '\0') {
        printf("L8 config: missing [client_name] parameter\n");
        missing_params ++;
    }
    if (conf->server_name[0] == '\0') {
        printf("L8 config: missing [server_name] parameter\n");
        missing_params ++;
    }
    if (conf->server_ip[0] == '\0') {
        printf("L8 config: missing [server_ip] parameter\n");
        missing_params ++;
    }
    if (conf->server_port == PARAM_INIT_VALUE) {
        printf("L8 config: missing [server_port] parameter\n");
        missing_params ++;
    }
    if (conf->local_bind_ip[0] == '\0') {
        printf("L8 config: missing [local_bind_ip] parameter\n");
        missing_params ++;
    }
    if (conf->local_bind_port == PARAM_INIT_VALUE) {
        printf("L8 config: missing [local_bind_port] parameter\n");
        missing_params ++;
    }
    if (conf->client_recv_delay == PARAM_INIT_VALUE) {
        printf("L8 config: missing [client_recv_delay] parameter\n");
        missing_params ++;
    }
    if (conf->server_frwd_delay == PARAM_INIT_VALUE) {
        printf("L8 config: missing [server_frwd_delay] parameter\n");
        missing_params ++;
    }
    if (conf->server_recv_delay == PARAM_INIT_VALUE) {
        printf("L8 config: missing [server_recv_delay] parameter\n");
        missing_params ++;
    }
    if (conf->client_frwd_delay == PARAM_INIT_VALUE) {
        printf("L8 config: missing [client_frwd_delay] parameter\n");
        missing_params ++;
    }
    return missing_params;

}

void config_display(struct L8Configuration * conf)
{
    printf("L8-Proxy configuration:\n");
    printf("[%s address] %s:%ld\n", conf->server_name, conf->server_ip, conf->server_port);
    printf("<<< %s %04ld millisecs\n", conf->client_name, conf->client_recv_delay);
    printf(">>> %s %04ld millisecs\n", conf->server_name, conf->server_frwd_delay);
    printf("<<< %s %04ld millisecs\n", conf->server_name, conf->server_recv_delay);
    printf(">>> %s %04ld millisecs\n", conf->client_name, conf->client_frwd_delay);
}

struct L8Configuration * config_create()
{
    int valid_config = 1;

    FILE * config_file = fopen(CONFIG_PATH, "r");
    if (config_file == 0) {
        printf("L8 config: load failure\n");
        perror(CONFIG_PATH);
        return NULL;
    }

    struct L8Configuration * conf = config_init();

    int invalid_lines = __parse_config_file(config_file, conf);
    if (invalid_lines) valid_config = 0;

    fclose(config_file);

    int missing_params = __check_missing_params(conf);
    if (missing_params) valid_config = 0;

    if (!valid_config) return NULL;

    config_display(conf);
    return conf;
}
