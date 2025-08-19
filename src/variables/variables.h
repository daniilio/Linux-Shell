/**
 * @file variables.h
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief This file contains definitions related to methods for running environment variables in the
 * shell 
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __VARIABLES_H__
#define __VARIABLES_H__

#include <unistd.h>


// Dynamic array sizing
#define GROWTH_FACTOR 2
#define BASE_SIZE 16

// structures for holding variables
// one variable entry
typedef struct {
    char *name;
    char *value;
} var;
// container for environment variables
typedef struct {
    var **var_arr;
    size_t capacity;
    size_t length;
} var_cont;


/**
 * @brief Determines whether a command string is a variable assignment.
 *
 * A variable assignment is identified if the string contains an '=' and has at least one character
 * before it (e.g., "VAR=value").
 *
 * @param cmd The command string to check.
 * @return size_t Returns 3 if the command is a variable assignment, or 2 otherwise.
 */
size_t var_check(char *cmd);

/**
 * @brief Parses an assignment command and stores the variable in the container.
 *
 * Splits a string of the form "NAME=VALUE" into its name and value components, and inserts them 
 * into the given variable container.
 *
 * @param vars Pointer to the variable container struct.
 * @param cmd  User-provided assignment string (e.g., "PATH=/usr/bin").
 * @return ssize_t 0 on success, or -1 on error.
 */
ssize_t assign_var(var_cont *vars, char *cmd);

/**
 * @brief Expands environment variables in an array of tokens.
 *
 * Iterates through all strings in @p token_arr, replacing any environment variable references 
 * (e.g., "$VAR") with their values from the given variable container. Each token is modified in 
 * place. Expansion halts if the cumulative length of expanded tokens exceeds `MAX_STR_LEN`, and the 
 * token array is truncated at that point.
 *
 * @param vars Pointer to the variable container used for lookups.
 * @param token_arr NULL-terminated array of command tokens to expand.
 *
 * @return size_t The number of tokens remaining in @p token_arr after expansion.
 */
size_t expand_vars(var_cont *vars, char **token_arr);

/**
 * @brief Creates and initializes a new variable container.
 *
 * Allocates memory for a variable container and its underlying array of variable pointers. The 
 * container is initialized with a capacity of BASE_SIZE and a length of 0.
 *
 * @return var_cont* Pointer to the newly allocated variable container on success, or NULL if memory 
 * allocation fails.
 *
 */
var_cont * create_vars(void);

/**
 * @brief Frees all memory associated with a variable container.
 *
 * @param vars Pointer to the variable container to free. Must not be NULL.
 *
 * @return void
 */
void free_vars(var_cont *vars);

#endif
