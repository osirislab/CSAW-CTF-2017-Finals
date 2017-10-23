#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <manager.h>

struct packed_thread_args
{
    manager_t *manager;
    int socket;
};

void* connection_handler(void *arg)
{
    struct packed_thread_args *args;
    FILE *socket;

    args = (struct packed_thread_args *) arg;
    socket = fdopen(args->socket, "w+");

    show_menu(args->manager, socket);

    return NULL;
}

int main(int argc, char *argv[]) 
{
    int socket_desc, client_sock;
    socklen_t c;
    struct sockaddr_in server, client;
    pthread_t thread_id;

    manager_t *manager;
    struct packed_thread_args args;

    if (argc != 2) {
        printf("Usage: connectXor [bind port]\n");
        exit(1);
    }

    manager = new_manager();

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Could not create socket\n");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return errno;
    }

    listen(socket_desc, BACKLOG);

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, &c))) {
        args.manager = manager;
        args.socket = client_sock;

        if (pthread_create(&thread_id, NULL, connection_handler, (void *)&args) < 0) {
            perror("Could not create thread\n");
            return errno;
        }
    }

    if (client_sock < 0) {
        perror("Accept failed");
        return errno;
    }

    free_manager(manager);
    return 0;
}
