/**
 * @file variables.c
 * @author Daniil Trukhin (daniilvtrukhin@gmail.com)
 * @brief This file contains methods for running environment variables in the shell.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "variables.h"
#include "io_helpers.h"


static void expand_str(var_cont *vars, char **token, size_t *total_len);
static ssize_t set_var(var_cont *vars, char *name, char *value);
static char * get_value(var_cont *vars, char *name);
static ssize_t set_new_var(var_cont *vars, char *name, char *value);
static ssize_t extend_vars(var_cont *vars);
static var * find_var(var_cont *vars, char *name);


/**
 * @brief Determines whether a command string is a variable assignment.
 *
 * A variable assignment is identified if the string contains an '=' and has at least one character
 * before it (e.g., "VAR=value").
 *
 * @param cmd The command string to check.
 * @return size_t Returns 3 if the command is a variable assignment, or 2 otherwise.
 */
size_t var_check(char *cmd) 
{
    if (strchr(cmd, '=') != NULL && strnlen(cmd, 2) > 1)
        return 3;
    return 2;
}

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
ssize_t assign_var(var_cont *vars, char *cmd) 
{
    char *cmd_cpy = strndup(cmd, MAX_STR_LEN + 1);
    char *name = strtok(cmd, "=");
    char *value = strchr(cmd_cpy, '=');
    value++;

    ssize_t err = set_var(vars, name, value);
    if (err == -1)
        return -1;

    free(cmd_cpy);

    return 0;
}

/**
 * @brief Expands environment variables in a token string.
 *
 * Searches the string referenced by @p token_ptr for variable references of the form `$VAR` and 
 * replaces them with their values from the given variable container. The expanded result overwrites 
 * the original string in-place. Updates @p total_len to reflect the cumulative length of the
 * processed tokens (with spaces between tokens counted).
 *
 * - Variables that are undefined expand to an empty string ("").
 * - Expansion stops once @p total_len reaches `MAX_STR_LEN`.
 *
 * @param vars Pointer to the variable container (used to look up values).
 * @param token_ptr Pointer to the string token to expand (modified in place).
 * @param total_len Pointer to the running total length of concatenated tokens.
 *
 * @return void
 */
static void expand_str(var_cont *vars, char **token_ptr, size_t *total_len) 
{
    char buffer[MAX_STR_LEN+1] = {'\0'};
    char *curr_ptr, *delimit_ptr, *value;
    curr_ptr = strchr(*token_ptr, '$');
    
    // exit if nothing to parse
    if (curr_ptr == NULL) {
        // don't concatenate if the total length has exceeded max
        if (*total_len >= MAX_STR_LEN) {
            return;
        }
        *total_len += strnlen(*token_ptr, MAX_STR_LEN - *total_len);
        // account for space in between tokens
        if (*total_len < MAX_STR_LEN) {
            (*total_len)++;
        }
        return;
    }

    // (*token_ptr)[0] = '\0';
    // case where characters appear before $  (i.e. a$X)
    if (curr_ptr != *token_ptr) {
        *curr_ptr = '\0';
        if (*total_len >= MAX_STR_LEN) {
            return;
        }
        strncat(buffer, *token_ptr, MAX_STR_LEN - *total_len);
        *total_len += strnlen(*token_ptr, MAX_STR_LEN - *total_len);
        *curr_ptr = '$';
    }

    while (curr_ptr != NULL) {
        // don't concatenate when max character limit has been reached
        if (*total_len >= MAX_STR_LEN) {
            break;
        }
        // check that next character is a valid variable name
        if (*(curr_ptr+1) == '$' || *(curr_ptr+1) == '\0') {
            strncat(buffer, "$", MAX_STR_LEN - *total_len);
            *total_len += strnlen("$", MAX_STR_LEN - *total_len);
            curr_ptr = strchr(curr_ptr+1, '$');
            continue;
        }

        // extract the variable name
        delimit_ptr = strchr(curr_ptr+1, '$');
        if (delimit_ptr != NULL) {
            *delimit_ptr = '\0';
            value = get_value(vars, curr_ptr+1);
            // revert back the delimiter
            *delimit_ptr = '$';
        } else {
            value = get_value(vars, curr_ptr+1);
        }

        // default to "" if variable not found
        if (value == NULL) {
            value = "";
        }

        // concatenate the variable value
        strncat(buffer, value, MAX_STR_LEN - *total_len);
        *total_len += strnlen(value, MAX_STR_LEN - *total_len);

        curr_ptr = strchr(curr_ptr+1, '$');
    }

    // set the token in token_arr to new value
    buffer[MAX_STR_LEN] = '\0';
    (*token_ptr)[0] = '\0';
    strncpy(*token_ptr, buffer, MAX_STR_LEN + 1);

    // account for space in between tokens
    if (*total_len < MAX_STR_LEN) {
        (*total_len)++;
    }
}

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
size_t expand_vars(var_cont *vars, char **token_arr) 
{
    size_t i = 0, total_len = 0;
    for (; token_arr[i] != NULL; i++) {
        if (total_len >= MAX_STR_LEN) {
            token_arr[i] = NULL;
            break;
        }
        // expand the args in the token
        expand_str(vars, token_arr + i, &total_len);
    }
    return i;
}

/**
 * @brief Frees all memory associated with a variable container.
 *
 * @param vars Pointer to the variable container to free. Must not be NULL.
 *
 * @return void
 */
void free_vars(var_cont *vars) 
{
    // free var_arr elements first
    var *variable;
    for (size_t i = 0; i < vars->length; i++) {
        variable = vars->var_arr[i];
        free(variable->name);
        free(variable->value);
        free(variable);
    }
    free(vars->var_arr);
    free(vars);
}

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
var_cont * create_vars(void) 
{
    var_cont *vars = malloc(sizeof(var_cont));
    if (vars == NULL)
        return NULL;

    vars->capacity = BASE_SIZE;
    vars->length = 0;

    vars->var_arr = calloc(vars->capacity, sizeof(var *));
    if (vars->var_arr == NULL) {
        free(vars);
        return NULL;
    }

    return vars;
}

/**
 * @brief Sets or updates a variable in the container.
 *
 * If the variable with the given name does not already exist in the container, a new variable is 
 * created and inserted. If it exists, its value is replaced with the new value.
 *
 * @param vars  Pointer to the variable container.
 * @param name  Variable name (C string). Must not be NULL.
 * @param value Variable value (C string). Must not be NULL.
 *
 * @return ssize_t 0 on success, or -1 on failure (e.g., memory allocation error).
 *
 */
static ssize_t set_var(var_cont *vars, char *name, char *value) 
{
    var *variable = find_var(vars, name);
    if (variable == NULL) {
        ssize_t err = set_new_var(vars, name, value);
        if (err == -1) {
            return -1;
        }
    } else {
        free(variable->value);

        size_t value_size = strnlen(value, MAX_STR_LEN) + 1;
        variable->value = calloc(value_size, sizeof(char));
        strncpy(variable->value, value, value_size);
        variable->value[value_size - 1] = '\0';
    }

    return 0;
}

/**
 * @brief Creates and inserts a new variable into the container.
 *
 * Allocates a new variable struct, copies the provided name and value into dynamically allocated 
 * memory, and appends the variable to the container. If the container is at capacity, it attempts 
 * to grow it using extend_vars().
 *
 * @param vars  Pointer to the variable container.
 * @param name  Variable name (C string). Must not be NULL.
 * @param value Variable value (C string). Must not be NULL.
 *
 * @return ssize_t 0 on success, or -1 on failure (e.g., memory allocation error or failed container 
 * resize).
 *
 */
static ssize_t set_new_var(var_cont *vars, char *name, char *value) 
{
    if (vars->length >= vars->capacity) {
        ssize_t err = extend_vars(vars);
        // failed to resize variable container
        if (err == -1)
            return -1;
    }

    size_t index = vars->length;
    vars->var_arr[index] = malloc(sizeof(var));
    var *variable = vars->var_arr[index];

    size_t name_size = strnlen(name, MAX_STR_LEN) + 1;
    variable->name = calloc(name_size, sizeof(char));
    strncpy(variable->name, name, name_size);
    variable->name[name_size - 1] = '\0';

    size_t value_size = strnlen(value, MAX_STR_LEN) + 1;
    variable->value = calloc(value_size, sizeof(char));
    strncpy(variable->value, value, value_size);
    variable->value[value_size - 1] = '\0';

    vars->length++;
    
    return 0;
}

/**
 * @brief Increases the capacity of a variable container.
 *
 * Resizes the container's underlying array of variable pointers by multiplying its capacity by 
 * 'GROWTH_FACTOR'. Copies all existing variable pointers into the new array, frees the old array, 
 * and updates the container metadata.
 *
 * @param vars Pointer to the variable container to resize.
 *
 * @return ssize_t 0 on success, or -1 on failure (e.g., memory allocation error or capacity 
 * overflow).
 *
 * @note This function does not modify the variable contents themselves,
 *       only reallocates the array of pointers.
 */
static ssize_t extend_vars(var_cont *vars) 
{
    size_t new_capacity = vars->capacity * GROWTH_FACTOR;
    // overflow (capacity too big)
    if (new_capacity < vars->capacity)
        return -1;

    // allocate new array
    var **new_var_arr = calloc(new_capacity, sizeof(var *));
    if (new_var_arr == NULL)
        return -1;

    // copy contents of original array to new array
    var *variable;
    for (size_t i = 0; i < vars->length; i++) {
        variable = vars->var_arr[i];
        new_var_arr[i] = variable;
    }
    free(vars->var_arr);

    vars->capacity = new_capacity;
    vars->var_arr = new_var_arr;

    return 0;
}

/**
 * @brief Retrieves the value of a variable by name.
 *
 * Searches the given variable container for a variable with the specified name and returns its 
 * value string.
 *
 * @param vars Pointer to the variable container.
 * @param name C string representing the variable name to look up.
 *
 * @return char* Pointer to the value string of the variable, or NULL if the variable does not exist 
 * in the container.
 *
 * @note The returned pointer points to the internal storage of the variable. Do not free or modify 
 * it directly.
 * 
 */
static char *get_value(var_cont *vars, char *name) 
{
    var *variable = find_var(vars, name);
    if (variable == NULL)
        return NULL;

    return variable->value;
}

/**
 * @brief Finds a variable in the container by name.
 *
 * Iterates through the variable container and searches for a variable whose name matches the given 
 * C string. Comparison is done up to 'MAX_STR_LEN' characters.
 *
 * @param vars Pointer to the variable container.
 * @param name C string representing the variable name to search for.
 *
 * @return var* Pointer to the variable struct with the matching name, or NULL if no such variable 
 * exists.
 *
 * @note The returned pointer points to the internal storage of the container. Do not free it 
 * directly.
 * 
 */
static var * find_var(var_cont *vars, char *name) 
{
    var *variable;
    for (size_t i = 0; i < vars->length; i++) {
        variable = vars->var_arr[i];
        if (strncmp(variable->name, name, MAX_STR_LEN) == 0)
            return variable;
    }
    
    return NULL;
}
