/**
 * @file client.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Header definitions for chat clients that can connect to a chat server.
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
 * @brief Runs the client program which allows a user to send messages to a chat server.
 * 
 * @param tokens Tokenized user command-line input.
 * @return ssize_t 0 upon success, -1 upon error.
 */
ssize_t client(char **tokens);

/**
 * @brief Sends a single message to a server.
 * 
 * @param tokens Tokenized user command-line input.
 * @return ssize_t 0 upon success, -1 upon error.
 */
ssize_t send_msg(char **tokens);

#endif
