#define MAX_CFG_VALUE_LEN   24

typedef struct L8Configuration {
    char client_name[MAX_CFG_VALUE_LEN];
    char server_name[MAX_CFG_VALUE_LEN];
    char server_ip[MAX_CFG_VALUE_LEN];
    long server_port;
    char local_bind_ip[MAX_CFG_VALUE_LEN];
    long local_bind_port;
    long client_recv_delay;
    long server_frwd_delay;
    long server_recv_delay;
    long client_frwd_delay;
} L8Config;

L8Config * config_create();
