#include <stdio.h>   // printf
#include <unistd.h>  // close
#include <pthread.h> // pthread_create, pthread_t
#include <limits.h>  // INT_MAX
#include <stdlib.h>  // strtol
#include <string.h>  // strcmp

#include "mutex.h"      // ExclusiveData, exclusive_data_flush
#include "config.h"     // L8Config, config_create
#include "connect.h"    // ConnectionParameters, conn_params_create

int main(int argc, char* argv[])
{
    L8Config * conf = config_create();
    if (!conf) return -1;

    int bnd_sock_fd = 0;
    int client_sock_fd = 0;
    int server_sock_fd = 0;

    bnd_sock_fd = local_bind(conf->local_bind_ip, conf->local_bind_port);
    if (bnd_sock_fd < 1) return -1;

    while (1) {

        // WAIT FOR CLIENT
        client_sock_fd = client_listen(bnd_sock_fd, conf->client_name);
        if (client_sock_fd < 1) return -1;

        // CONNECT TO SERVER
        server_sock_fd = server_connect(conf->server_ip, conf->server_port, conf->server_name);
        if (server_sock_fd < 1) return -1;

        // ALLOW INTERTHREAD-COMMUNICATION OF PROXY-SERVER STATUS
        struct ExclusiveData * ProxyStatus = exclusive_data_create("ON", 2);

        // INITIALIZE FORWARDING DATA BUFFERS
        struct ExclusiveData * ClientData = exclusive_data_create("", 0);
        struct ExclusiveData * ServerData = exclusive_data_create("", 0);

        // SET THREAD PARAMETERS
        pthread_t th1;
        struct ConnectionParameters * client_connection_recv = conn_params_create(
            conf->client_name, client_sock_fd, conf->client_recv_delay, ClientData, ProxyStatus, 0
        );

        pthread_t th2;
        struct ConnectionParameters * server_connection_frwd = conn_params_create(
            conf->server_name, server_sock_fd, conf->server_frwd_delay, ClientData, ProxyStatus, 0
        );

        pthread_t th3;
        struct ConnectionParameters * server_connection_recv = conn_params_create(
            conf->server_name, server_sock_fd, conf->server_recv_delay, ServerData, ProxyStatus, 4
        );

        pthread_t th4;
        struct ConnectionParameters * client_connection_frwd = conn_params_create(
            conf->client_name, client_sock_fd, conf->client_frwd_delay, ServerData, ProxyStatus, 4
        );

        // START PROXYING DATA
        pthread_create(&th1, NULL, connection_recv, client_connection_recv);
        pthread_create(&th2, NULL, connection_frwd, server_connection_frwd);
        pthread_create(&th3, NULL, connection_recv, server_connection_recv);
        pthread_create(&th4, NULL, connection_frwd, client_connection_frwd);
        printf("L8-Proxy started\n");

        // STOP  PROXYING DATA
        pthread_join(th1, NULL);
        pthread_join(th2, NULL);
        pthread_join(th3, NULL);
        pthread_join(th4, NULL);
        printf("L8-Proxy stopped\n");

        // CLOSE SESSION
        if (client_sock_fd != 0) {
            close(client_sock_fd);
            client_sock_fd = 0;
        }

        if (server_sock_fd != 0) {
            close(server_sock_fd);
            server_sock_fd = 0;
        }

        printf("Cleaning-up\n");

        free(client_connection_recv);
        free(server_connection_frwd);
        free(server_connection_recv);
        free(client_connection_frwd);

        exclusive_data_flush(ClientData);
        exclusive_data_flush(ServerData);

        free(ServerData);
        free(ClientData);
        free(ProxyStatus);

    }

    if (bnd_sock_fd != 0) {
        // shutdown(bnd_sock_fd);
        close(bnd_sock_fd);
    }

    printf("Done.\n");
    return 0;

}
