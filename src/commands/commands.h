/**
 * @file commands.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Definitions for methods that implement bash commands.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <unistd.h>

#define MAX_FILE_NAME 512
// provide large buffer for handling directory expansions  (e.g., ..., ....)
#define MAX_BACK_PATH 512

/**
 * @brief Prints user input tokens to standard output.
 *
 * Iterates through the tokens array starting from index 1 and prints each token separated by 
 * spaces.
 *
 * @param tokens NULL-terminated array of user input tokens.
 *
 * @return ssize_t 0 on success, or -1 if an error occurs.
 *
 */ 
ssize_t cmd_echo(char **tokens);

/**
 * @brief Executes a custom "ls" command with optional flags and recursion.
 *
 * Parses user input tokens for supported flags (`--f`, `--rec`, `--d`) and a single directory path, 
 * then lists directory contents accordingly.
 * 
 * Behaviour:
 * - `--f <substring>`: filters files containing the substring.
 * - `--rec`: enables recursive listing.
 * - `--d <depth>`: limits recursion depth (requires `--rec`).
 * - No path provided defaults to the current directory ("./").
 *
 * @param tokens NULL-terminated array of user input tokens.
 *
 * @return ssize_t 0 on success, -1 on error (e.g., invalid flags, missing arguments, invalid path, 
 * or recursive listing failure).
 *
 * @note Depth argument of -1 means unlimited recursion.
 * 
 */
ssize_t cmd_ls(char **tokens);

/**
 * @brief Changes the current working directory.
 *
 * Implements a custom "cd" command. If no path is provided, changes to the user's HOME directory. 
 * Supports processing of dot sequences in the path (e.g., "...." => "/../../..").
 *
 * @param tokens NULL-terminated array of user input tokens.
 *
 * @return ssize_t 0 on success or -1 on error (e.g., failed chdir).
 *
 */
ssize_t cmd_cd(char **tokens);

/**
 * @brief Displays the contents of a file or standard input.
 *
 * Implements a custom "cat" command. If a file path is provided in @p tokens[1], the function reads 
 * and prints its contents. If no path is provided, reads from standard input (stdin) until EOF.
 *
 * @param tokens NULL-terminated array of user input tokens.
 *
 * @return ssize_t 0 on success, -1 on error (e.g., cannot open file, too many arguments).
 *
 */
ssize_t cmd_cat(char **tokens);

/**
 * @brief Counts words, characters, and newlines in a file or standard input.
 *
 * Implements a custom "wc" command. If a file path is provided in @p tokens[1], the function reads 
 * and counts its contents. If no path is provided, reads from standard input (stdin) until EOF.
 *
 * @param tokens NULL-terminated array of user input.
 * 
 * @return ssize_t 0 on success, -1 on error (e.g., cannot open file or too many arguments).
 *
 */
ssize_t cmd_wc(char **tokens);

/**
 * @brief Sends a signal to a specified process.
 *
 * Implements a custom "kill" command. Sends a signal to the process identified by the PID provided 
 * in @p tokens[1]. If no signal is specified in @p tokens[2], defaults to SIGTERM.
 *
 * @param tokens NULL-terminated array of user input tokens, where: @p tokens[0] is "kill", 
 * @p tokens[1] is the PID of the target process, and @p tokens[2] (optional) is the signal number 
 * to send.
 *
 * @return ssize_t 0 on success, -1 on error (e.g., missing PID, invalid PID, non-existent process, 
 * invalid signal, or failed kill).
 *
 */
ssize_t cmd_kill(char **tokens);

/**
 * @brief Lists all background processes launched by the shell.
 *
 * @param process_arr Array of process IDs.
 * @param process_name_arr  Array of strings containing the command names corresponding to the 
 * processes in process_arr.
 *
 * @return ssize_t 0 on success, -1 on error.
 *
 * @note Only processes with a valid PID (not -1) are displayed. Output is printed as 
 * "<command> <pid>".
 */
ssize_t cmd_ps(int *process_arr, char **process_name_arr);

#endif
