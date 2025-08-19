/**
 * @file chat_helpers.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Helper methods for handling server client management, writing to sockets, and reading
 * from sockets.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"
#include "chat_helpers.h"

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
int write_buf_to_client(struct client_sock *c, char *buf, int len) 
{
    // check that buf is null terminated
    if (strchr(buf, '\0') == NULL) {
        return 1;
    }
    if (len > BUF_SIZE-2) {
        return 1;
    }
    buf[len] = '\r';
    buf[len+1] = '\n';
    return write_to_socket(c->sock_fd, buf, len+2);
}

/**
 * @brief Removes a client from the linked-list of clients.
 * 
 * @param curr Pointer to the client to be removed.
 * @param clients Pointer to head of the clients linked-list.
 * @return int 0 upon successo or 1 upon failure.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients) 
{
    if (clients == NULL || curr == NULL) {
        return 1;
    }
    if (*clients == NULL || *curr == NULL) {
        return 1;
    }

    struct client_sock *remove = *curr;  // curr is in the front of the list
    if (*clients == *curr) {
        *clients = (*clients)->next;
        *curr = *clients;
        
        free(remove);
        return 0;
    }
    
    struct client_sock *prev = *clients;
    while (prev->next != NULL) {
        if (prev->next == *curr) {
            prev->next = (*curr)->next;
            *curr = prev->next;
            
            free(remove);
            return 0;
        }
        prev = prev->next;
    }
    
    return 1; // Couldn't find the client in the list, or empty list
}

/**
 * @brief Read incoming bytes from a client.
 * 
 * @param curr Pointer to client socket struct.
 * @return int 0 upon reciept of CRLF-terminated message, 1 if the client socket is closed, 2
 * upon reciept of partial (non-CRLF-terminated) message, or -1 upon read error or maximum message
 * size exceeded.
 */
int read_from_client(struct client_sock *curr) 
{
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}
