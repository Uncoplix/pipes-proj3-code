#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * make use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) {
    // Check index bounds
    if ((in_idx < -1 || in_idx >= 2 * n_pipes) || (out_idx < -1 || out_idx >= 2 * n_pipes)) {
        fprintf(stderr, "Index out of bounds\n");
        return -1;
    }

    // Set up stdin redirection if applicable
    if (in_idx != -1) {
        // Safely redirect stdin to read end of specified pipe
        if (dup2(pipes[in_idx], STDIN_FILENO) == -1) {
            perror("dup2");    // errno set
            return -1;
        }
    }

    // Set up stdout redirection if applicable
    if (out_idx != -1) {
        // Safely redirect stdout to write end of specified pipe
        if (dup2(pipes[out_idx], STDOUT_FILENO) == -1) {
            perror("dup2");    // errno set
            return -1;
        }
    }

    // Safely run the command after redirection has been taken care of
    if (run_command(tokens) == -1) {
        fprintf(stderr, "run_command\n");
        return -1;
    }

    /* Caller is responsible for closing any ends of any pipes */
    /* The function has made it to the end successfully */
    /* Should not reach this point if run_command succeeds */
    return 0;
}

int run_pipelined_commands(strvec_t *tokens) {
    /* START PIPE SETUP BLOCK */
    // Safely allocate and create an array of pipes
    int num_pipes = strvec_num_occurrences(tokens, "|");
    if (num_pipes == 0) {
        /* There are no pipes so save time and execute like normal */
        // Safely run the command that has no pipes
        if (run_command(tokens) == -1) {
            fprintf(stderr, "run_command\n");
            return -1;
        }
        return 0;
    }
    int *pipes_fds = malloc(2 * num_pipes * sizeof(int));
    if (pipes_fds == NULL) {
        perror("malloc");
        return -1;
    }
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes_fds + 2 * i) == -1) {
            perror("pipe");
            // Close old pipes up to i'th pipe
            for (int j = 0; j < i; j++) {
                close(pipes_fds[2 * j]);
                close(pipes_fds[2 * j + 1]);
            }
            free(pipes_fds);    // Free up that malloc :p
            return -1;
        }
    }
    /* END PIPE SETUP BLOCK */

    // Fork children for each command
    int num_commands = num_pipes + 1;
    for (int i = num_commands - 1; i >= 0; i--) {
        pid_t child_pid = fork();

        // Starts from the last command (decremements after child logic)
        int pipe_idx = strvec_find_last(tokens, "|");

        if (child_pid == -1) {
            perror("fork");    // errno set

            // Close all pipes
            for (int j = 0; j < num_pipes; j++) {
                close(pipes_fds[2 * j]);
                close(pipes_fds[2 * j + 1]);
            }

            free(pipes_fds);    // Free up that malloc :p

            // Wait until all already existing children die
            for (int j = 0; j < i; j++) {
                wait(NULL);
            }
            return -1;
        } else if (child_pid == 0) {
            /* Child process */

            // Determine I/O pipes for helper
            int in_idx = -1;
            int out_idx = -1;
            if (i == 0) {
                /* First command outputs to write end of first pipe */
                out_idx = 1;
            } else if (i == num_commands - 1) {
                /* Last command takes input from read end of last pipe */
                in_idx = 2 * (num_pipes - 1) + 0;
            } else {
                /* All commands between first and last pipe */
                in_idx = 2 * (i - 1);    // Read to pipe connecting to last command
                out_idx = 2 * i + 1;     // Write to pipe connecting to next command
            }
            // Close all pipes that the child isn't using
            int close_error = 0;
            for (int j = 0; j < num_pipes; j++) {
                if (in_idx != 2 * j) {
                    if (close(pipes_fds[2 * j]) == -1) {
                        close_error = -1;
                    };
                }
                if (out_idx != 2 * j + 1) {
                    if (close(pipes_fds[2 * j + 1]) == -1) {
                        close_error = -1;
                    };
                }
            }
            if (close_error == -1) {
                perror("close");
                if (in_idx != -1) {
                    close(pipes_fds[in_idx]);
                }
                if (out_idx != -1) {
                    close(pipes_fds[out_idx]);
                }
                exit(1);
            }
            // Free the non needed strvec
            strvec_t cmd_tokens;
            if (strvec_slice(tokens, &cmd_tokens, pipe_idx + 1, tokens->length) == -1) {
                fprintf(stderr, "strvec_slice\n");
                if (in_idx != -1) {
                    close(pipes_fds[in_idx]);
                }
                if (out_idx != -1) {
                    close(pipes_fds[out_idx]);
                }
                exit(1);
            }

            // Safely execute piped command
            if (run_piped_command(&cmd_tokens, pipes_fds, num_pipes, in_idx, out_idx) == -1) {
                fprintf(stderr, "run_piped_commands\n");
                strvec_clear(&cmd_tokens);
                if (in_idx != -1) {
                    close(pipes_fds[in_idx]);
                }
                if (out_idx != -1) {
                    close(pipes_fds[out_idx]);
                }
                exit(1);
            }

            /* Should not reach this point if run_piped_commands succeeds */
            exit(0);
        }

        // Remove the command that was just used from tokens
        strvec_take(tokens, pipe_idx);
    }

    // Close both ends of all pipes
    for (int i = 0; i < num_pipes; i++) {
        close(pipes_fds[2 * i]);
        close(pipes_fds[2 * i + 1]);
    }
    free(pipes_fds);    // Free up that malloc :p

    // Wait for all children
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }

    return 0;
}
