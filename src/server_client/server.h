/**
 * @file server.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Header definitions for a chat server that hosts multiple clients.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/types.h> 

/**
 * @brief Runs a chat server that can hold multiple clients concurrently.
 * 
 * @param tokens user input tokens
 * @param server_fd read end of pipe connected to main program
 * @return ssize_t 0 upon success, -1 upon error.
 */
ssize_t server(char **tokens, int server_fd);

#endif
