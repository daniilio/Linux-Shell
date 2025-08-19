/**
 * @file client.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Contains methods for running a client program and messaging program that communicate with 
 * chat servers.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "io_helpers.h"
#include "socket.h"
#include "chat_helpers.h"

static ssize_t _read_user_input(struct server_sock s, char *write_buf, int prepend_len, 
                                int *write_len);
static ssize_t _recieve_message(struct server_sock *s, char **msg);
static ssize_t _get_client_id(struct server_sock *s, int *client_id);
static ssize_t _parse_port_num(char **tokens, int *server_port);
static ssize_t _verify_user_input(char **tokens);
static ssize_t _client_setup(struct server_sock *s, char **tokens, int server_port);

/**
 * @brief Runs the client program which allows a user to send messages to a chat server.
 * 
 * @param tokens Tokenized user command-line input.
 * @return ssize_t 0 upon success, -1 upon error.
 */
ssize_t client(char **tokens)
{
    if (_verify_user_input(tokens) == -1) {
        return -1;
    }

    clearerr(stdin); // ensure stdin is not at EOF

    // parse the port number
    int server_port;
    if (_parse_port_num(tokens, &server_port) == -1) {
        return -1;
    }

    struct server_sock s;
    if (_client_setup(&s, tokens, server_port) == -1) {
        return -1;
    }

    // get the client id from the server
    int client_id;
    int ret;
    if ( (ret = _get_client_id(&s, &client_id)) == 1 ) {
        return 0;
    } else if (ret == -1) {
        return -1;
    }

    // prepend client id to messages
    char write_buf[BUF_SIZE];
    snprintf(write_buf, BUF_SIZE, "client%d:", client_id);
    int prepend_len = strnlen(write_buf, BUF_SIZE);

    fd_set fdset;
    FD_ZERO(&fdset);

    while (feof(stdin) == 0) {
        FD_ZERO(&fdset);
        FD_SET(s.sock_fd, &fdset);
        FD_SET(STDIN_FILENO, &fdset);
        if (select(s.sock_fd + 1, &fdset, NULL, NULL, NULL) < 0) {
            perror("ERROR: select");
            close(s.sock_fd);
            return -1;
        }

        // read user entered messages and send to the server
        if ( FD_ISSET(STDIN_FILENO, &fdset) ) {
            char msg_buf[BUF_SIZE];
            int write_len;
            if (_read_user_input(s, write_buf, prepend_len, &write_len) == 1) {
                return 0;
            }

            // send message to the server
            int ret;
            if ( (ret = write_to_socket(s.sock_fd, write_buf, write_len)) == 1 ) {
                display_error("ERROR: ", "Server write failure");
                close(s.sock_fd);
                return -1;
            }
            if (ret == 2) {
                display_error("ERROR: ", "Server disconnected");
                close(s.sock_fd);
                return 0;
            }

            // reset write_buf
            snprintf(write_buf, BUF_SIZE, "client%d:", client_id);
        }

        // recieve server messages
        if ( FD_ISSET(s.sock_fd, &fdset) ) {
            char *msg;
            _recieve_message(&s, &msg);
            strncat(msg, "\n", 2);
            display_message(msg); // write the server message to the user
            free(msg);
        }
    }

    close(s.sock_fd);
    return 0;
}

/**
 * @brief Reads user input from stdin into buffer 'write_buf'.
 * 
 * @param s Server socket struct.
 * @param write_buf User input buffer.
 * @param prepend_len Length of client id prepend.
 * @return ssize_t 0 upon success or 1 if stdin is closed.
 * @note 'write_buf' must contain enough space for the input and the prepended client id.
 */
static ssize_t _read_user_input(struct server_sock s, char *write_buf, int prepend_len, 
                                int *write_len) 
{
    char msg_buf[BUF_SIZE];
    if (fgets(msg_buf, (MAX_USER_MSG + 2) - prepend_len, stdin) == NULL) {
        // stdin closed
        close(s.sock_fd);
        return 1;
    }
    strncat(write_buf, msg_buf, (MAX_USER_MSG + 2) - prepend_len);

    char *nl;
    if ( (nl = strchr(write_buf, '\n')) != NULL ) {
        *write_len = strnlen(write_buf, MAX_USER_MSG + 1) + 1; // include network newline
        *nl = '\r';                                           // append network newline
        *(nl + 1) = '\n';
    } else {
        *write_len = MAX_USER_MSG + 2;
        ungetc(write_buf[MAX_USER_MSG], stdin);
        write_buf[MAX_USER_MSG] = '\r'; // append network newline
        write_buf[MAX_USER_MSG + 1] = '\n';
    }

    return 0;
}

/**
 * @brief Recieve a message from server socket and parse it into buffer 'msg'.
 * 
 * @param s Server socket struct.
 * @param msg Pointer to message.
 * @return ssize_t 0 upon success, 1 if invalid data recieved.
 * @note the message is parsed into a valid c string with the network newline.
 * @note 'msg' is allocated heap memory.
 * @note Disconnects client from the server if invalid data is recieved.
 */
static ssize_t _recieve_message(struct server_sock *s, char **msg)
{
    int client_closed;
    while ( (client_closed = read_from_socket(s->sock_fd, s->buf, &s->inbuf)) != 0 ) {
        if (client_closed == -1) {
            display_error("ERROR: ", "server read error");
            close(s->sock_fd);
            return -1;
        } else if (client_closed == 1) {
            display_error("ERROR: ", "Server disconnected");
            close(s->sock_fd);
            return 1;
        }
    }

    int msg_len = -1;
    while (msg_len <= 2) {
        if ( (client_closed == 0) && !get_message(msg, s->buf, &(s->inbuf)) ) {
            msg_len = strnlen(*msg, MAX_PROTO_MSG);

            if (msg_len <= 2) {
                free(*msg);
                continue;
            } else if (msg_len > MAX_PROTO_MSG) {
                display_error("ERROR: ", "Server disconnected");
                free(*msg);
                close(s->sock_fd);
                return 1;
            }

            (*msg)[msg_len - 2] = '\0'; // remove network newline
        }
    }

    return 0;
}

/**
 * @brief Recieves the client id sent by the server and stores it to 'client_id' pointer.
 * 
 * @param s Server socket struct.
 * @param client_id Pointer to client id.
 * @return ssize_t 0 upon success, 1 if invalid data is recieved, or -1 upon error.
 * @note Disconnects client from the server if invalid data is recieved.
 */
static ssize_t _get_client_id(struct server_sock *s, int *client_id) 
{
    char *msg;
    int ret;
    if ( (ret = _recieve_message(s, &msg)) == -1 ) {
        return -1;
    }
    if (ret == 1) {
        return 1;
    }

    errno = 0; // convert the client_id to an integer
    *client_id = strtol(msg, NULL, 10);
    if (errno != 0) {
        display_error("ERROR: ", "Server disconnected");
        free(msg);
        close(s->sock_fd);
        return -1;
    }
    free(msg);

    return 0;
}

/**
 * @brief Parse server port number to update 'server_port' pointer.
 *
 * @param tokens Tokenized user command-line input.
 * @param server_port Pointer to server port number.
 * @return ssize_t 0 upon success, -1 upon error.
 */
static ssize_t _parse_port_num(char **tokens, int *server_port)
{
    errno = 0;
    *server_port = strtol(tokens[1], NULL, 10);
    if (errno != 0) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    return 0;
}

/**
 * @brief Validates user command-line input.
 *
 * @param tokens Tokenized user command-line input.
 * @return ssize_t 0 if no user error found, -1 if user error occurs.
 */
static ssize_t _verify_user_input(char **tokens)
{
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    if (tokens[2] == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }
    if (tokens[3] != NULL) {
        display_error("ERROR: ",
                      "Too many arguments: start-client takes a single port-number and hostname");
        return -1;
    }
    return 0;
}

/**
 * @brief Sets up the client socket and connects to the server.
 * 
 * @param s Server socket struct.
 * @param tokens Tokenized user command-line input.
 * @param server_port Server port number.
 * @return ssize_t 0 upon success, -1 upon error.
 */
static ssize_t _client_setup(struct server_sock *s, char **tokens, int server_port) 
{
    s->inbuf = 0;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("client: socket");
        return -1;
    }

    // set the IP and port of the server to connect to
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    if (inet_pton(AF_INET, tokens[2], &server.sin_addr) < 1) {
        if (errno != 0) { // system error
            perror("client: inet_pton");
        } else { // usage error
            display_error("ERROR: ", "No hostname provided");
        }
        close(s->sock_fd);
        return -1;
    }

    // connect to the server
    if (connect(s->sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("ERROR: client: connect");
        close(s->sock_fd);
        return -1;
    }

    return 0;
}

/**
 * @brief Sends a single message to a server.
 * 
 * @param tokens Tokenized user command-line input.
 * @return ssize_t 0 upon success, -1 upon error.
 */
ssize_t send_msg(char **tokens)
{
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    if (tokens[2] == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }

    // get port number
    errno = 0;
    int server_port = strtol(tokens[1], NULL, 10);
    if (errno != 0) {
        display_error("ERROR: No port provided", "");
        return -1;
    }

    struct server_sock s;
    if (_client_setup(&s, tokens, server_port) == -1) {
        return -1;
    }

    // parse user message into 'write_buf'
    char write_buf[BUF_SIZE] = {'\0'};
    int append_total = 0;
    for (int i = 3; tokens[i] != NULL && BUF_SIZE - append_total > 0; i++) {
        strncat(write_buf, tokens[i], BUF_SIZE - append_total);
        append_total += strnlen(tokens[i], MAX_STR_LEN);
        strncat(write_buf, " ", 2);
        append_total += 1;
    }
    strncat(write_buf, "\r\n", 3);
    int msg_len = strnlen(write_buf, BUF_SIZE);

    // send message to server
    int ret;
    if ((ret = write_to_socket(s.sock_fd, write_buf, msg_len)) == 1) {
        perror("ERROR: write_to_socket");
        close(s.sock_fd);
        return -1;
    }
    if (ret == 2) {
        display_error("ERROR: ", "Server disconnected");
        close(s.sock_fd);
        return -1;
    }

    close(s.sock_fd);
    return 0;
}
