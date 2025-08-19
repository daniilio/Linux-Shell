/**
 * @file socket.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Server/client helper methods for reading and writing to sockets.
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
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "socket.h"

/**
 * @brief Searches the first n character of a buffer for network-newline (CRLF).
 * 
 * @param buf The buffer to search in.
 * @param inbuf Amount of characters in the buffer.
 * @return int 1 plus the index of the '\n' of the network-newline (CRLF) or -1 if not found.
 */
int find_network_newline(const char *buf, int inbuf) 
{
    for (int i = 0; i < inbuf - 1; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return i + 2;
        }
    }
    return -1;
}

/**
 * @brief Reads from the socket file descriptor into buffer.
 * 
 * 'inbuf' is updated to reflect the number of characters inside the buffer. Data is never
 * over-written in the buffer.
 * 
 * @param sock_fd Socket file descriptor.
 * @param buf The buffer to write to.
 * @param inbuf Pointer to number of characters inside the buffer.
 * @return int 0 upon receipt of CRLF-terminated message, 1 if the socket has been closed, 
 * 2 upon receipt of partial (non-CRLF-terminated) message, or -1 if read error or maximum message
 * size is exceeded.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf) 
{    
    int bytes_read;
    if (*inbuf >= BUF_SIZE) {
        return -1;
    }
    if ((bytes_read = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf)) == -1) {
        return -1;
    }

    *inbuf = *inbuf + bytes_read;
    if (bytes_read == 0) {
        return 1;
    }

    int crlf = find_network_newline(buf, *inbuf);
    if (crlf != -1 && crlf < BUF_SIZE) {
        return 0;
    }
    if (*inbuf >= BUF_SIZE) {
        return -1;
    }
    return 2;
}

/**
 * @brief Searches src for a network newline, and copies complete message into a newly-allocated
 * NULL-terminated string 'dst'. Also removes the complete message from the 'src' buffer by moving
 * the remaining content of the buffer to the front.
 * 
 * @param dst Destination buffer.
 * @param src Source buffer.
 * @param inbuf Number of characters in 'src' buffer.
 * @return int 0 upon success or 1 upon error.
 */
int get_message(char **dst, char *src, int *inbuf) 
{
    if (src == NULL) {
        return 1;
    }

    int crlf;
    if ((crlf = find_network_newline(src, *inbuf)) == -1) {
        return 1;
    }

    *dst = calloc(BUF_SIZE, sizeof(char));

    memcpy(*dst, src, crlf);
    (*dst)[crlf] = '\0';

    if (*inbuf > 0) {
        memmove(src, src + crlf, *inbuf - crlf);
    }
    *inbuf = *inbuf - crlf;

    return 0;
}

/**
 * @brief Writes a string to a socket.
 * 
 * @param sock_fd Socket file descriptor.
 * @param buf Buffer contain string to write.
 * @param len Length of the string.
 * @return int 0 upon success, 1 upon error, or 2 upon disconnect.
 */
int write_to_socket(int sock_fd, char *buf, int len) 
{
    if (buf == NULL) {
        return 1;
    }

    int total_written = 0;
    while (total_written < len) {
        int bytes_written = write(sock_fd, buf + total_written, len - total_written);
        if (bytes_written == -1) {
            perror("ERROR: write");
            return 1;
        }
        if (bytes_written == 0) {
            return 2;
        }
        total_written += bytes_written;
    }
    
    return 0;
}
