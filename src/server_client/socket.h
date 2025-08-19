/**
 * @file socket.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Definitions for sever/client helper methods used in reading and writing to sockets.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _SOCKET_H_
#define _SOCKET_H_

#ifndef MAX_BACKLOG
    #define MAX_BACKLOG 100
#endif

#ifndef MAX_USER_MSG
    #define MAX_USER_MSG 128
#endif

// 2 bytes for CRLF
#ifndef MAX_PROTO_MSG
    #define MAX_PROTO_MSG MAX_USER_MSG+2
#endif

// 1 byte for the null terminator
#ifndef BUF_SIZE
    #define BUF_SIZE MAX_PROTO_MSG+1
#endif

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

struct listen_sock {
    struct sockaddr_in *addr;
    int sock_fd;
};

/**
 * @brief Searches the first n character of a buffer for network-newline (CRLF).
 * 
 * @param buf The buffer to search in.
 * @param inbuf Amount of characters in the buffer.
 * @return int 1 plus the index of the '\n' of the network-newline (CRLF) or -1 if not found.
 */
int find_network_newline(const char *buf, int n);

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
int read_from_socket(int sock_fd, char *buf, int *inbuf);

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
int get_message(char **dst, char *src, int *inbuf);

/**
 * @brief Writes a string to a socket.
 * 
 * @param sock_fd Socket file descriptor.
 * @param buf Buffer contain string to write.
 * @param len Length of the string.
 * @return int 0 upon success, 1 upon error, or 2 upon disconnect.
 */
int write_to_socket(int sock_fd, char *buf, int len);
 
#endif
