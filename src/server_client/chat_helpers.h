/**
 * @file chat_helpers.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef CHAT_HELPERS_H
#define CHAT_HELPERS_H

struct client_sock {
    int sock_fd;
    int state;
    char buf[BUF_SIZE];
    int inbuf;
    struct client_sock *next;
};

/**
 * @brief Sends a string to a client.
 * 
 * Input buffer must contain a NULL-terminated string. The NULL terminator is replaced with a
 * network-newline (CRLF) before being sent to the client.
 * 
 * @param c Pointer to client socket struct.
 * @param buf Buffer containing the string to send.
 * @param len The length of the message (not including network-newline).
 * @return int 0 upon success, 1 upon error, or 2 upon client disconnect.
 */
int write_buf_to_client(struct client_sock *c, char *buf, int len);

/**
 * @brief Removes a client from the linked-list of clients.
 * 
 * @param curr Pointer to the client to be removed.
 * @param clients Pointer to head of the clients linked-list.
 * @return int 0 upon successo or 1 upon failure.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients);

/**
 * @brief Read incoming bytes from a client.
 * 
 * @param curr Pointer to client socket struct.
 * @return int 0 upon reciept of CRLF-terminated message, 1 if the client socket is closed, 2
 * upon reciept of partial (non-CRLF-terminated) message, or -1 upon read error or maximum message
 * size exceeded.
 */
int read_from_client(struct client_sock *curr);

#endif
