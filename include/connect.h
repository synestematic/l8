#define MAX_HOSTNAME_LEN    16
#define MAX_INDENT_LEN      8

int local_bind(char * ip, int port);

int client_listen(int bnd_sock_fd, char * client_name);

int server_connect(char * ip, int port, char * server_name);

struct ConnectionParameters {
    char host[MAX_HOSTNAME_LEN];
    char indent[MAX_INDENT_LEN];
    int sock;
    long delay; // msecs
    struct ExclusiveData * data_buffer;
    struct ExclusiveData * proxy_status;
};

struct ConnectionParameters * conn_params_create(char * host, int sock, long msec_delay, struct ExclusiveData * data, struct ExclusiveData * active, int indent);

void* connection_recv(void * conn_params);

void* connection_frwd(void * conn_params);
