#include <stdio.h>      // printf
#include <stdlib.h>     // NULL
#include <string.h>     // strerror, strlen
#include <errno.h>      // errno
#include <unistd.h>     // usleep

#include <sys/socket.h> // socket
#include <sys/select.h> // select
#include <sys/types.h>  // size_t

#include <arpa/inet.h>  // inet_addr
#include <netinet/in.h> // sockaddr

#include "mutex.h"      // exclusive_data_copy
#include "connect.h"    // ConnectionParameters, MAX_HOSTNAME_LEN, MAX_INDENT_LEN

int local_bind(char * ip, int port)
{
    int bnd_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (bnd_sock_fd < 0) {
        printf("Failed to open client socket\n");
        return -1;
    };

    int enable = 1;
    if (setsockopt(bnd_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("Failed to set listening socket option\n");
        return -1;
    };

    struct sockaddr_in bnd_address;
    bnd_address.sin_family = AF_INET;
    bnd_address.sin_port = htons(port);
    bnd_address.sin_addr.s_addr = inet_addr(ip);

    int bnd_status = -1;
    bnd_status = bind(
        bnd_sock_fd, (struct sockaddr *) &bnd_address, sizeof(bnd_address)
    );
    if (bnd_status == -1) {
        printf("Failed to bind fd[%d] to %s:%d  (%s)\n", bnd_sock_fd, ip, port, strerror(errno));
        usleep(1000 * 1000);
        return -1;
    }

    printf("%s:%d bound to fd[%d]\n", ip, port, bnd_sock_fd);
    return bnd_sock_fd;
}


int client_listen(int bnd_sock_fd, char * client_name)
{
    printf("Waiting for %s to connect...\n", client_name);
    listen(bnd_sock_fd, 1);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int client_sock_fd = accept(
        bnd_sock_fd, (struct sockaddr *) &client_address, &client_address_len
    );

    printf(
        "%s connected from %s:%d\n",
        client_name,
        inet_ntoa(client_address.sin_addr),
        (int) ntohs(client_address.sin_port)
    );

    return client_sock_fd;
}


int server_connect(char * ip, int port, char * server_name)
{
    printf("Connecting to %s... \n", server_name);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip);

    socklen_t server_address_len = sizeof(server_address);

    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_fd < 0) {
        printf("Failed to open server socket\n");
        return -1;
    };

    int status = connect(
        server_sock_fd, (struct sockaddr *) &server_address, server_address_len
    );
    if (status != 0) {
        printf("Failed to connect to %s!\n", server_name);
        return -1;
    }

    return server_sock_fd;
}


struct ConnectionParameters * conn_params_create(char* host, int sock, long msec_delay, struct ExclusiveData * data, struct ExclusiveData * status, int tabs)
{
    struct ConnectionParameters * new_conn = malloc(sizeof(struct ConnectionParameters));

    int ho = strlen(host);
    if (ho > MAX_HOSTNAME_LEN) ho = MAX_HOSTNAME_LEN;
    new_conn->host[0] = '\0';
    for (int h = 0; h < ho; h++) {
        new_conn->host[h] = host[h];
    }

    if (tabs > MAX_INDENT_LEN) tabs = MAX_INDENT_LEN;
    new_conn->indent[0] = '\0';
    for (int i = 0; i < tabs; i++) {
        new_conn->indent[i] = '\t';
    }

    new_conn->sock = sock;
    new_conn->delay = msec_delay;
    new_conn->data_buffer = data;
    new_conn->proxy_status = status;
    return new_conn;
}


void* connection_recv(void* conn_params)
{
    struct ConnectionParameters* recv_params = conn_params;
    int recv_sock_fd = recv_params->sock;
    long usecs_delay = recv_params->delay * 1000; // convert millisecs to microsecs

    fd_set readfds;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(recv_sock_fd, &all_fds);

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;

    for (;;) {

        if ( exclusive_data_size(recv_params->proxy_status) == 0 ) return NULL;

        readfds = all_fds;

        select(recv_sock_fd + 1, &readfds, NULL, NULL, &tv);

        if (FD_ISSET(recv_sock_fd, &readfds)) {

            ssize_t recv_bytes = 0;
            char recv_data[4096]; // stackoverflow.com/questions/2862071/how-large-should-my-recv-buffer-be-when-calling-recv-in-the-socket-library//
            if (usecs_delay) usleep(usecs_delay);    // usec: microseconds
            recv_bytes = recv(recv_sock_fd, &recv_data, sizeof(recv_data), 0);

            // APPEND TO BUFFER
            if (recv_bytes) {

                append_to_exclusive_data(recv_params->data_buffer, recv_data, recv_bytes);

                printf("%sGot  %ld bytes from %s\n", recv_params->indent, recv_bytes, recv_params->host);

            } else {

                printf("%s disconnected\n", recv_params->host);

                // RESET PROXY_STATUS WILL MAKE OTHER THREADS RETURN
                exclusive_data_flush(recv_params->proxy_status);

                return NULL;

            }

        }

    }

    return NULL;

}

void* connection_frwd(void* conn_params)
{
    struct ConnectionParameters* send_params = conn_params;
    int send_sock_fd = send_params->sock;
    long usecs_delay = send_params->delay * 1000; // convert millisecs to microsecs

    fd_set writefds;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(send_sock_fd, &all_fds);

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;

    for (;;) {

        if ( exclusive_data_size(send_params->proxy_status) == 0 ) return NULL;

        writefds = all_fds;

        select(send_sock_fd + 1, NULL, &writefds, NULL, &tv);

        if (FD_ISSET(send_sock_fd, &writefds) ) {

            // RETRIEVE FROM BUFFER AND FLUSH

            char send_data[4096];
            long send_bytes = exclusive_data_copy(send_data, send_params->data_buffer);

            if (send_bytes == -1) printf("%sFailed to read data buffer\n", send_params->indent);

            // DELAY AND SEND DATA, ONLY IF ANY
            if (send_bytes) {
                if (usecs_delay) usleep(usecs_delay);    // usec: microseconds
                size_t sent_bytes = send(send_sock_fd, send_data, send_bytes, 0);

                if (sent_bytes != send_bytes) {
                    printf("%sSent %ld bytes instead of %ld\n", send_params->indent, sent_bytes, send_bytes);

                } else {
                    printf("%sSent %ld bytes to %s\n", send_params->indent, sent_bytes, send_params->host);

                }

            }

        }

    }

    return NULL;
}
