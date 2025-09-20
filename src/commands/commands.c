/**
 * @file commands.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief Contains methods that implement bash commands.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>

#include "commands.h"
#include "io_helpers.h"
#include "builtins.h"


static int _digits_only(const char *s);
static int _is_hidden_file(const char *file);
static ssize_t _list_dir(char *path, char *f_substr, bool rec, int depth);
static ssize_t _detect_flags(char **f_substr, bool *rec, int *depth, char **path, char **tokens);
static ssize_t _process_path(char *path, char *new_path);


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
ssize_t cmd_echo(char **tokens) 
{
    ssize_t index = 1;

    if (tokens[index] != NULL) {
        display_message(tokens[index]);
        index += 1;
    }
    while (tokens[index] != NULL) {
        display_message(" ");
        display_message(tokens[index]);

        index += 1;
    }
    display_message("\n");

    return 0;
}

/**
 * @brief Checks if a string contains numerical digits only.
 * 
 * @param s c string to check.
 * 
 * @return int 1 if the string contains only digits, 0 otherwise.
 * 
 */
static int _digits_only(const char *s) 
{
    while (*s) {
        if (isdigit(*s++) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Determines if a given file name represents a hidden file.
 *
 * A hidden file is defined as a file whose name begins with a dot ('.'), but is not "." or "..".
 *
 * @param file c string representing the file name.
 *
 * @return int 1 if the file is hidden, 0 otherwise.
 *
 */
static int _is_hidden_file(const char *file) 
{
    if (file == NULL) {
        return 0;
    }
    if (file[0] == '.' && file[1] != '\0' && file[1] != '.') {
        return 1;
    }
    return 0;
}

/**
 * @brief Recursively lists directory contents with optional filtering.
 *
 * Iterates through the files and subdirectories of the given path, printing names that contain the 
 * substring @p f_substr and are not hidden (do not start with a dot). Optionally performs recursive 
 * listing up to the specified depth.
 *
 * @param path Path to the directory to list.
 * @param f_substr Substring filter (entries not containing this substring won't be printed).
 * @param rec Boolean flag to indicate whether recursion into subdirectories should be performed.
 * @param depth Maximum depth of recursion. Use -1 for unlimited depth. depth == 0 prints only the 
 * current directory indicator (".").
 *
 * @return ssize_t 0 on success, or -1 if an error occurs.
 *
 * @note Depth counting defines the current directory as level 1.
 * 
 */
static ssize_t _list_dir(char *path, char *f_substr, bool rec, int depth) 
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    if (depth == 0) {
        display_message(".\n");
        (void) closedir(dir);
        return 0;
    }

    struct dirent *ep;
    char buffer[MAX_FILE_NAME];
    char full_path[MAX_BACK_PATH];
    
    while ((ep = readdir(dir)) != NULL) {
        if (strstr(ep->d_name, f_substr) != NULL && !(_is_hidden_file(ep->d_name))) {
            snprintf(buffer, sizeof(buffer), "%s\n", ep->d_name);
            display_message(buffer);
        }

        // check if recursion to be performed
        if ( rec &&
            (depth > 1 || depth == -1) &&
            (ep->d_type == DT_DIR) &&
            (strncmp(ep->d_name, "..", 3) != 0) &&
            (strncmp(ep->d_name, ".", 2) != 0) &&
            !(_is_hidden_file(ep->d_name)) ) {  // don't perform recursion on back directories
        
            if (path[strlen(path) - 1] == '/')
                snprintf(full_path, sizeof(full_path), "%s%s", path, ep->d_name);
            else
                snprintf(full_path, sizeof(full_path), "%s/%s", path, ep->d_name);

            ssize_t err;
            if (depth == -1) {
                err = _list_dir(full_path, f_substr, rec, -1);
            } else {
                err = _list_dir(full_path, f_substr, rec, depth - 1);
            }
            if (err == -1) {
                (void) closedir(dir);
                return -1;
            }
        }
    }

    (void) closedir(dir);

    return 0;
}

/**
 * @brief Parses user input tokens to detect command flags and arguments.
 *
 * Examines the provided tokens array to identify supported flags:
 * - `--f <substring>`: sets the substring filter for directory listings.
 * - `--rec`: toggles recursion on or off.
 * - `--d <depth>`: sets the maximum recursion depth (integer).
 *
 * Stores detected flag values in the corresponding output pointers.
 *
 * @param f_substr Pointer to store the `--f` substring filter.
 * @param rec      Pointer to store the recursion flag (true=perform recursion).
 * @param depth    Pointer to store the recursion depth.
 * @param path     Pointer to store the directory path string.
 * @param tokens   NULL-terminated array of user input tokens.
 *
 * @return ssize_t 0 on successful parsing, -1 if an error occurs (e.g., missing flag arguments, 
 * invalid depth, too many paths, or unrecognized flag).
 *
 * @note Only a single path is allowed. The function validates that depth arguments are numeric and 
 * that required flag arguments are present.
 * 
 */
static ssize_t _detect_flags(char **f_substr, bool *rec, int *depth, char **path, char **tokens) 
{
    *path = NULL;
    for (size_t i = 1; tokens[i] != NULL; i++) {
        if (strncmp(tokens[i], "--", 2) == 0) {
            char *flag = tokens[i] + 2;
            
            if (strncmp(flag, "f", 2) == 0) {  // display all directory contents that contain substring
                if (tokens[i + 1] == NULL || strncmp(tokens[i + 1], "--", 2) == 0) {
                    display_error("ERROR: Missing arguments: ", "--f flag takes subsequent substring");
                    return -1;
                }

                *f_substr = tokens[i + 1];
                i++;  // skip the subsequent substring
            } else if (strncmp(flag, "rec", 4) == 0) {
                *rec = !(*rec);
            } else if (strncmp(flag, "d", 2) == 0) {
                if (tokens[i + 1] == NULL || strncmp(tokens[i + 1], "--", 2) == 0) {
                    display_error("ERROR: Missing arguments: ", "--d flag takes subsequent search depth");
                    return -1;
                }
                if (!_digits_only(tokens[i + 1])) {
                    display_error("ERROR: --d flag takes an integer search depth: ", tokens[i + 1]);
                    return -1;
                }
                
                *depth = strtol(tokens[i + 1], NULL, 10);
                i++;  // skip the subsequent search depth
            } else {
                display_error("ERROR: Unrecognized flag option: ", tokens[i]);
                return -1;
            }
        } else {
            if (*path != NULL) {
                display_error("ERROR: Too many arguments: ", "ls takes a single path");
                return -1;
            }
            *path = tokens[i];
        }
    }

    return 0;
}

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
ssize_t cmd_ls(char **tokens) 
{
    char *f_substr = "";
    bool rec = false;
    int depth = -1;  // depth -1 means recurse to bottom of the tree
    char *path = NULL;

    if (_detect_flags(&f_substr, &rec, &depth, &path, tokens) == -1) {
        return -1;
    }

    if (!rec && depth > -1) {
        display_error("ERROR: Invalid flag option: ", "--d must be provided with --rec");
        return -1;
    }
    
    if (path == NULL) {
        path = "./";
    }
    
    if (_list_dir(path, f_substr, rec, depth) == -1) {
        return -1;
    }

    return 0;
}

/**
 * @brief Expands dot sequences in a directory path string.
 *
 * Converts sequences of dots in the input path into corresponding parent directory references. For 
 * example, "...." becomes "/../../..". The result is written into the provided buffer 
 * @p `new_path`.
 *
 * @param path Input path string to process. Must be a valid C string.
 * @param new_path Buffer to store the expanded path. Must have sufficient space to hold the result 
 * (e.g., size >= 'MAX_BACK_PATH' bytes).
 *
 * @return ssize_t 0 on success, -1 on error.
 *
 */
static ssize_t _process_path(char *path, char *new_path) 
{
    char *trail_ptr = NULL;
    // len + 1 to account for trailing .. at the end of path
    for (size_t i = 0; i < strnlen(path, MAX_STR_LEN) + 1; i++) {
        if (path[i] == '.') {
            if (trail_ptr == NULL) trail_ptr = &path[i];
        } else if (trail_ptr != NULL) {
            for (size_t q = 0; q < 2 && trail_ptr != &path[i]; q++) {  // skip first .    .
                strncat(new_path, ".", 2);
                trail_ptr++;
            }
            for (; trail_ptr != &path[i]; trail_ptr++) {
                strncat(new_path, "/..", 4);
            }
            trail_ptr = NULL;
            new_path[strnlen(new_path, MAX_BACK_PATH)] = path[i];
            new_path[strnlen(new_path, MAX_BACK_PATH)] = '\0';
        } else {
            new_path[strnlen(new_path, MAX_BACK_PATH)] = path[i];
            new_path[strnlen(new_path, MAX_BACK_PATH)] = '\0';
        }
    }
    
    return 0;
}

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
ssize_t cmd_cd(char **tokens) 
{
    char *path = tokens[1];
    if (path == NULL) {
        path = getenv("HOME");
    } else {
        if (tokens[2] != NULL) {
            display_error("ERROR: Too many arguments: ", "cd takes a single path");
            return 0;
        }
    }
    
    char new_path[MAX_BACK_PATH] = {'\0'};
    _process_path(path, new_path);  // process any ... sequence
    path = new_path;
    
    DIR *dir = opendir(path);  // check if valid directory
    if (dir == NULL) {
        display_error("ERROR: Invalid path", "");
        return 0;
    }
    (void) closedir(dir);

    int err = chdir(path);
    if (err == -1)
        return -1;

    return 0;
}

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
ssize_t cmd_cat(char **tokens) 
{
    char *path = tokens[1];
    FILE *file;

    if (path == NULL) {
        file = stdin;
        clearerr(stdin);
    } else {
        if (tokens[2] != NULL) {
            display_error("ERROR: Too many arguments: ", "cat takes a single file");
            return -1;
        }
        file = fopen(tokens[1], "r");
    }

    if (file == NULL) {
        display_error("ERROR: Cannot open file", "");
        return -1;
    }

    char buffer[2];
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        snprintf(buffer, sizeof(buffer), "%c", ch);
        display_message(buffer);
    }
    if (file != stdin) {
        fclose(file);
    }

    return 0;
}

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
ssize_t cmd_wc(char **tokens) 
{
    char *path = tokens[1];
    FILE *file;
    
    if (path == NULL) {
        file = stdin;
        clearerr(stdin);
    } else {
        if (tokens[2] != NULL) {
            display_error("ERROR: Too many arguments: ", "wc takes a single file");
            return -1;
        }
        file = fopen(tokens[1], "r");
    }

    if (file == NULL) {
        display_error("ERROR: Cannot open file", "");
        return -1;
    }

    char ch;
    bool wd_f = true;
    size_t wd_c = 0, ch_c = 0, nl_c = 0;
    while ((ch = fgetc(file)) != EOF) {
        ch_c++;
        
        if (ch == '\n') nl_c++;

        if (isspace(ch)) {
            wd_f = true;
        } else if (wd_f) {
            wd_c++;
            wd_f = false;
        }
    }

    char buffer[MAX_STR_LEN];
    snprintf(buffer, sizeof(buffer), "word count %zu\n", wd_c);
    display_message(buffer);

    snprintf(buffer, sizeof(buffer), "character count %zu\n", ch_c);
    display_message(buffer);

    snprintf(buffer, sizeof(buffer), "newline count %zu\n", nl_c);
    display_message(buffer);

    if (file != stdin) {
        fclose(file);
    }
    
    return 0;
}

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
ssize_t cmd_kill(char **tokens) 
{
    int pid;
    int signal;

    if (tokens[1] == NULL) {
        display_error("ERROR: No Process Provided", "");
        return -1;
    }
    errno = 0;
    pid = strtol(tokens[1], NULL, 10);
    if (errno != 0) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }
    if (kill(pid, 0) != 0) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }

    if (tokens[2] == NULL) {
        signal = SIGTERM;
    } else {
        errno = 0;
        signal = strtol(tokens[2], NULL, 10);
        if (errno != 0) {
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
        struct sigaction act;
        if (sigaction(signal, NULL, &act) != 0) {
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
    }

    if (kill(pid, signal) == -1) {
        display_error("ERROR: kill", "");
        return -1;
    }

    return 0;
}

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
ssize_t cmd_ps(int *process_arr, char process_name_arr[][MAX_STR_LEN]) 
{
    char msg[1024] = {'\0'};
    char cmd_name[512];
    for (int i = 0; i < MAX_BACKGROUND_PROCESS; i++) {
        if (process_arr[i] != -1) {
            // get the command name
            int q;
            for (q = 0; process_name_arr[i][q] != ' '; q++) {
                cmd_name[q] = process_name_arr[i][q];
            }
            cmd_name[q] = '\0';

            // create msg
            snprintf(msg, sizeof(msg), "%s %d\n", cmd_name, process_arr[i]);
            display_message(msg);
        }
    }
    return 0;
}
