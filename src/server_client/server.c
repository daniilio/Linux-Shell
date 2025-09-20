/**
 * @file server.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Methods for running a chat server that hosts multiple clients.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "io_helpers.h"
#include "socket.h"
#include "chat_helpers.h"
#include "server_setup.h"

static void _clean_exit(struct listen_sock s, struct client_sock *clients, int server_fd);
static int _fd_close(int fd);
static int _accept_connection(int fd, struct client_sock **clients, struct client_sock **newclient);
static void _remove_client(int *client_count, int client_fd, fd_set *all_fds);
static ssize_t _send_client_id(int *client_id, int *client_count, struct client_sock *new_client);
static ssize_t _parse_port_num(char **tokens, int *server_port, int server_fd);
static int _server_commands(struct client_sock *clients, char *msg, int data_len, int client_count);
static void _process_client_data(struct client_sock *clients, struct fd_set *listen_fds, 
                          struct fd_set *all_fds, int *client_count);

/**
 * @brief Runs a multi-client chat server.
 * 
 * @param tokens Tokenized command-line input.
 * @param server_fd File descriptor for the read end of a pipe connected to the main program.
 * @return ssize_t Returns 0 on normal server shutdown, -1 on error.
 */
ssize_t server(char **tokens, int server_fd)
{
    int client_id = 0;
    int client_count = 0;

    int server_port;
    if (_parse_port_num(tokens, &server_port, server_fd) == -1) {
        return -1;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("server: signal");
        close(server_fd);
        return -1;
    }

    struct client_sock *clients = NULL; // linked list of clients

    struct listen_sock s;
    if (setup_server_socket(&s, server_port) == -1) {
        _clean_exit(s, clients, server_fd);
        return -1;
    }

    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;

    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    FD_SET(server_fd, &all_fds);
    if (server_fd > max_fd) {
        max_fd = server_fd;
    }

    int exit_status = 0;

    while (1) {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("server: select");
            exit_status = -1;
            break;
        }

        // if a new client is connecting, create new client_sock struct and add to clients
        // linked list
        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            struct client_sock *new_client;
            int client_fd = _accept_connection(s.sock_fd, &clients, &new_client);
            if (client_fd < 0) {
                continue;
            }
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);

            int ret = _send_client_id(&client_id, &client_count, new_client);
            if (ret == -1) {
                exit_status = -1;
                break;
            } else if (ret == 1) {
                _remove_client(&client_count, client_fd, &all_fds);
                continue;
            }
        }

        // check if the server is closed by main program
        if (FD_ISSET(server_fd, &listen_fds)) {
            int server_closed;
            if ((server_closed = _fd_close(server_fd)) < 0) {
                exit_status = -1;
                break;
            } else if (server_closed) {
                exit_status = 0;
                break;
            }
        }

        // accept incoming messages from clients, and send to all other connected clients
        _process_client_data(clients, &listen_fds, &all_fds, &client_count);
    }

    _clean_exit(s, clients, server_fd);
    return exit_status;
}

/**
 * @brief Closes all sockets, file descriptors, and frees all memory.
 * 
 * @param s The listening socket structure containing the server socket and its address.
 * @param clients Head of the linked list of connected clients.
 * @param server_fd File descriptor used by the main program to communicate with the server.
 */
static void _clean_exit(struct listen_sock s, struct client_sock *clients, int server_fd)
{
    struct client_sock *tmp;
    while (clients) {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp);
    }
    close(server_fd);
    close(s.sock_fd);
    free(s.addr);
}

/**
 * @brief Determines if a file descriptor has been closed by the other end.
 * 
 * Attempts to read a single byte from the given file descriptor to check for closure.
 * 
 * @param fd File descriptor to check.
 * @return int Returns 1 if the file descriptor is closed, 0 if it is still open and readable, -1 if 
 * an error occurred while reading.
 */
static int _fd_close(int fd)
{
    char buf;
    ssize_t bytes_read;
    do {
        errno = 0;
        bytes_read = read(fd, &buf, 1);
    } while (bytes_read < 0 && errno == EINTR);
    
    if (bytes_read < 0) {
        perror("read");
        return -1;
    } else if (bytes_read == 0) {
        return 1;
    }
    return 0;
}

/**
 * @brief Wait for and accept a new client connection.
 * 
 * Accepts a new client, allocates a new client_sock struct, and appends it to the linked list of 
 * clients.
 * 
 * @param fd Listening socket file descriptor.
 * @param clients Pointer to the head of the linked list of clients.
 * @param newclient Pointer to store the newly allocated client.
 * @return int The file descriptor of the new client, or -1 on error.
 */
static int _accept_connection(int fd, struct client_sock **clients, struct client_sock **newclient)
{
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    if (client_fd < 0) {
        perror("ERROR: server: accept");
        return -1;
    }

    *newclient = malloc(sizeof(struct client_sock));
    if (*newclient == NULL) {
        perror("server: malloc");
        close(client_fd);
        return -1;
    }

    (*newclient)->sock_fd = client_fd;
    (*newclient)->inbuf = 0;
    (*newclient)->state = 0;
    (*newclient)->next = NULL;
    memset((*newclient)->buf, 0, BUF_SIZE);

    // append new client to the end of the linked list
    if (*clients == NULL) {
        *clients = *newclient;
    }
    else {
        struct client_sock *curr = *clients;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = *newclient;
    }

    return client_fd;
}

/**
 * @brief Removes a client from the server.
 * 
 * @param client_count Pointer to the current number of connected clients.
 * @param client_fd File descriptor of the client to be removed.
 * @param all_fds Pointer to the fd_set used by select() for tracking active client sockets.
 */
static void _remove_client(int *client_count, int client_fd, fd_set *all_fds)
{
    *client_count -= 1;
    FD_CLR(client_fd, all_fds);
    close(client_fd);
}

/**
 * @brief Sends a unique client ID to a newly connected client.
 * 
 * @param client_id Pointer to the current client ID counter.
 * @param client_count Pointer to the current number of connected clients.
 * @param new_client Pointer to the struct representing the new client socket.
 * @return ssize_t Returns 0 on success, 1 if the client disconnected before receiving the ID,
 *                 or -1 if an error occurred while sending.
 */
static ssize_t _send_client_id(int *client_id, int *client_count, struct client_sock *new_client)
{
    *client_id += 1;
    *client_count += 1;

    char write_buf[BUF_SIZE];
    snprintf(write_buf, BUF_SIZE, "%d", *client_id);
    int msg_len = strnlen(write_buf, BUF_SIZE);

    int ret;
    if ((ret = write_buf_to_client(new_client, write_buf, msg_len)) == 1) {
        perror("ERROR: write_to_socket");
        return -1;
    }
    if (ret == 2) { // client disconnected
        return 1;
    }
    return 0;
}

/**
 * @brief Validates user input and parses the server port number.
 * 
 * Checks the tokens provided by the user for a valid server port argument. On success, the port 
 * number is stored in `server_port`. If an error occurs, the function displays an error message and 
 * closes `server_fd`.
 * 
 * @param tokens Array of command-line tokens provided by the user.
 * @param server_port Pointer to an integer where the parsed server port number will be stored.
 * @param server_fd File descriptor used by the main program to communicate with the server.
 * @return ssize_t Returns 0 on success, or -1 if a user input error occurs.
 */
static ssize_t _parse_port_num(char **tokens, int *server_port, int server_fd) 
{
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        close(server_fd);
        return -1;
    }
    if (tokens[2] != NULL) {
        display_error("ERROR: Too many arguments: start-server takes a single port number", "");
        close(server_fd);
        return -1;
    }

    // parse the server port number
    errno = 0;
    *server_port = strtol(tokens[1], NULL, 10);
    if (errno != 0) {
        display_error("ERROR: No port provided", "");
        close(server_fd);
        return -1;
    }

    return 0;
}

/**
 * @brief Handles server commands from a client message.
 * 
 * @param clients Pointer to the first client in the linked list.
 * @param msg The client message string.
 * @param data_len Length of the message (excluding network newline).
 * @param client_count Number of currently connected clients.
 * @return int Returns 0 if the "\connected" command was not found, 1 if the command was processed 
 * successfully, 2 if the command was found but sending the response failed.
 */
static int _server_commands(struct client_sock *clients, char *msg, int data_len, int client_count) 
{
    msg[data_len - 2] = '\0'; // remove network newline
    char *cmd = strchr(msg, ':');
    if ( (cmd != NULL) && (*(cmd + 1) != '\0') && 
         (strncmp(cmd + 1, "\\connected", strlen("\\connected") + 1) == 0) ) {
        char cmd_reply[BUF_SIZE];
        snprintf(cmd_reply, BUF_SIZE, "%d", client_count);
        int reply_len = strnlen(cmd_reply, BUF_SIZE);

        if (write_buf_to_client(clients, cmd_reply, reply_len) == 2) {
            return 2;
        }
        return 1;
    }
    return 0;
}

/**
 * @brief Processes incoming messages from all clients.
 * 
 * Reads incoming data from ready sockets, handles special commands, sends messages to other 
 * clients, and cleans up disconnected ones.
 * 
 * @param clients Pointer to the head of the linked list of clients.
 * @param listen_fds Pointer to fd_set containing sockets ready for reading (from select()).
 * @param all_fds Pointer to fd_set containing all active sockets, including the server socket.
 * @param client_count Pointer to the current number of connected clients.
 */
static void _process_client_data(struct client_sock *clients, struct fd_set *listen_fds, 
                                 struct fd_set *all_fds, int *client_count) 
{
    while (clients) {
        if (!FD_ISSET(clients->sock_fd, listen_fds)) {
            clients = clients->next;
            continue;
        }
        int client_closed = read_from_client(clients);

        // if error encountered when receiving data
        if (client_closed == -1) {
            client_closed = 1; // disconnect the client
        }

        char *msg;
        // loop through buffer to get complete message(s)
        while ( (client_closed == 0) && !get_message(&msg, clients->buf, &(clients->inbuf)) ) {
            char write_buf[BUF_SIZE];
            snprintf(write_buf, MAX_USER_MSG, "%s", msg);
            int data_len = strlen(write_buf);

            // check for "\connected" command            
            int ret = _server_commands(clients, msg, data_len, *client_count);
            free(msg);
            if (ret > 0) {
                if (ret == 2) {
                    _remove_client(client_count, clients->sock_fd, all_fds);
                }
                break;
            }

            display_message(write_buf); // display message to server operator

            // send the message to all other clients
            struct client_sock *dest_c = clients;
            while (dest_c) {
                if (write_buf_to_client(dest_c, write_buf, data_len) == 2) {
                    _remove_client(client_count, dest_c->sock_fd, all_fds);
                    continue;
                }
                dest_c = dest_c->next;
            }
        }

        if (client_closed == 1) { // client disconnected
            _remove_client(client_count, clients->sock_fd, all_fds);
        }
        else {
            clients = clients->next;
        }
    }
}
