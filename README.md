# Linux-Shell
A fully functional Linux command-line interface implemented in C!

---

⚠️ **NOTE:** Not all files in this project are included, as some starter code was provided as part of a class assignment I do not hold the copyright to.

## Overview of Features
- Custom implementations of **built-in commands** such as `ps`, `kill`, `cd`, `ls`, `echo`, `cat`, `wc`, and `exit`.
- TCP **chat server** capable of handling multiple clients simultaneously.
- TCP **chat client** that communicates with a chat server.
- Assignment and expansion of **environment variables**.
- **Backgrounding processes** with `&`.
- **Piping** with `|`.
- Graceful **error handling** for invalid commands, paths, or arguments.

## Commands Usage

- **`|`**
  
  **Usage:** `[command 1] | [command 2] | [command 3]  | ... | [command n]`
  
  **Description:** Pipes the output of `[command 1]` to `[command 2]` and `[command 2]` to `[command 3]` until `[command n]`.

- **`&`**
  
  **Usage:** `[command] &`
  
  **Description:** Runs the command in the background, listing a job number and process ID. Notifies the user when the command completes.

- **`ps`**
  
  **Usage:** `ps`
  
  **Description:** Lists process names and IDs for all background processes launched by the shell.- **`|`**

- **`kill`**
  
  **Usage:** `kill [pid] [signal number]`
  
  **Description:** Sends signal `[signal number]` to a process with PID `pid`. Defaults to SIGTERM if no signal is specified.

- **`cd`**
  
  **Usage:** `cd [path]`
  
  **Description:** Changes the current working directory to `[path]`. If no path is provided, defaults to the user's home directory. Supports
  backdirectory shortcuts using multiple dots, e.g., `..`, `...`, `....`, to navigate up multiple levels.

- **`ls`**
  
  **Usage:** `ls [path] [flags]`
  
  **Description:** Lists the contents of `[path]`, or the current directory if no path is provided. Supports flags such as:
  - `--f [substring]`: Only display entries containing `[substring]`.
  - `--rec`: Perform recursive listing.
  - `--d [depth]`: Limit recursion to `[depth]` levels.

- **`echo`**
  
  **Usage:** `echo [string]`
  
  **Description:** Outputs the `[string]` component to standard output (stdout).

- **`cat`**
  
  **Usage:** `cat [file]`
  
  **Description:** Displays the contents of `[file]` or standard input if no file is provided.

- **`wc`**
  
  **Usage:** `wc [file]`
  
  **Description:** Counts words, characters, and newlines in `[file]` or standard input.

- **`exit`**
  
  **Usage:** `exit`
  
  **Description:** Terminates the shell program with exit code 0.

## Server & Client Usage

- **`start-server`**
  
  **Usage:** `start-server [port number]`
  
  **Description:** Initiates a background chat server on `[port number]`.
  
- **`close-server`**
  
  **Usage:** `close-server`
  
  **Description:** Terminates active background servers launched by the shell.
  
- **`send`**
  
  **Usage:** `send [port number] [hostname] [message]`
  
  **Description:** Send `[message]` to `[hostname]` at `[port number]`.
  
- **`start-client`**
  
  **Usage:** `start-client [port number] [hostname]`
  
  **Description:** Starts a client that can send multiple messages to `[hostname]` at `[port number]`. The client is connected once and can continue
  sending messages until pressing CTRL+D or CTRL+C.

## Environment Variable Usage

- **Environment Variables**
  
  **Setting:** `myvar1=value`
  
  **Expansion:** `$myvar1` expands to `value`

## Signals Usage

- **`SIGINT`**
  
  **Description:** The `SIGINT` signal is ignored by the shell program.
