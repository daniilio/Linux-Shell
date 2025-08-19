/**
 * @file builtins.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Contains helper methods for running builtin bash commands (commands that are not 
 * internally implemented).
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "commands.h"


/**
 * @brief Checks if a command exists as an executable in common system paths.
 *
 * Constructs potential full paths for the command in `/bin/` and `/usr/bin/`
 * and checks if the file is executable. If found, the full path is written
 * into `bsh_path`.
 *
 * @param cmd The command name to check.
 * @param bsh_path Buffer to store the full path if the command is found. Must be at least 
 * 'MAX_FILE_NAME' bytes.
 *
 * @return ssize_t 1 if the command exists and is executable, 0 otherwise.
 *
 */
ssize_t check_bash(const char *cmd, char *bsh_path) 
{
    snprintf(bsh_path, MAX_FILE_NAME, "/bin/%s", cmd);
    if (access(bsh_path, X_OK) != -1)
        return 1;
    snprintf(bsh_path, MAX_FILE_NAME, "/usr/bin/%s", cmd);
    if (access(bsh_path, X_OK) != -1)
        return 1;
    return 0;
}

/**
 * @brief Determines if a command should run in the background.
 *
 * Checks whether the last token in the user input is a standalone '&', indicating a background 
 * process request.
 *
 * @param token_arr NULL-terminated array of user input tokens.
 * @param token_count Number of tokens in token_arr.
 *
 * @return ssize_t 1 if the command is a background process, 0 otherwise.
 *
 */
ssize_t check_background(char **token_arr, int token_count) 
{
    if (token_count >= 1) {
        char *last_token = token_arr[token_count - 1];
        if (strncmp(last_token, "&", 2) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Executes a builtin Bash command in a child process.
 *
 * Forks the current process to run the command specified by @p bsh_path with the given @p tokens as 
 * arguments. The parent process waits for the child to finish and returns its exit status.
 *
 * @param tokens NULL-terminated array of command arguments.
 * @param bsh_path Full path to the executable command.
 *
 * @return ssize_t Exit status of the executed command, or -1 if an error occurs.
 *
 */
ssize_t bsh_cmd(char **tokens, char *bsh_path) 
{
    int ret = fork();
    if (ret == -1) {
        perror("fork");
        return -1;
    } else if (ret == 0) {
        execv(bsh_path, tokens);
    } else {
        int status;
        wait(&status);
        return WEXITSTATUS(status);
    }
    return -1;
}
