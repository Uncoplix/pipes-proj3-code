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
    // TODO create implementation
    return 0;
}

int run_pipelined_commands(strvec_t *tokens) {
    // create an array of pipes
    int num_pipes = strvec_num_occurrences(tokens, "|");
    int *pipes_fds = malloc(2 * num_pipes * sizeof(int));
    for (int i = 0; i < num_pipes; i++) {
        pipe(pipes_fds + 2 * i);
    }

    // fork children for each command
    int num_commands = num_pipes + 1;
    for (int i = 0; i < num_commands; i++) {
        pid_t child_pid = fork();

        if (child_pid < 0) {
            perror("fork()");
            free(pipes_fds);
            return -1;
        } else if (child_pid == 0) {
            // child process

            // determine I/O
            int in_idx, out_idx = -1;
            if (i == 0) {
                // first command outputs to write end of first pipe
                out_idx = 1;
            } else if (i == num_commands - 1) {
                // last command takes input from read end of last pipe
                in_idx = 2 * (num_pipes - 1);
            } else {
                // middle commands
                in_idx = 2 * (i - 1);    // read pipe connecting to last command
                out_idx = 2 * i + 1;     // write to pipe connecting to next command
            }

            // TODO parse tokens for individual commands
            strvec_t cmd_tokens;
            strvec_init(&cmd_tokens);
            // PARSING LOGIC HERE

            // execute command
            if (run_piped_command(&cmd_tokens, pipes_fds, num_pipes, in_idx, out_idx) == -1) {
                fprintf(stderr, "run_piped_commands\n");
            }

            // should not reach this point if run_piped_commands() succeeds
            free(pipes_fds);
            return -1;
        }
    }

    free(pipes_fds);
    return 0;
}
