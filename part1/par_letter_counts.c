#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ALPHABET_LEN 26

/*
 * Counts the number of occurrences of each letter (case insensitive) in a text
 * file and stores the results in an array.
 * file_name: The name of the text file in which to count letter occurrences
 * counts: An array of integers storing the number of occurrences of each letter.
 *     counts[0] is the number of 'a' or 'A' characters, counts [1] is the number
 *     of 'b' or 'B' characters, and so on.
 * Returns 0 on success or -1 on error.
 */
int count_letters(const char *file_name, int *counts) {
    // Counts has length 26
    // Use case insensitive logic
    // printf("%s\n", file_name);
    FILE *fh = fopen(file_name, "r");
    if (fh == NULL) {
        perror("fopen");    // errno set
        return -1;
    }

    char letter;
    while (fread(&letter, sizeof(char), 1, fh) > 0) {
        if (!isalpha(letter)) {
            continue;
        }
        char lower = tolower(letter);    // Letter is in alphabet, make it lowercase.
        counts[lower - 'a'] += 1;
    }

    // Error check read
    if (ferror(fh)) {
        perror("fread");    // errno set
        fclose(fh);
        return -1;
    }

    // Safely close file
    if (fclose(fh) == EOF) {
        perror("fclose");    // errno set
        return -1;
    };

    return 0;
}

/*
 * Processes a particular file(counting occurrences of each letter)
 *     and writes the results to a file descriptor.
 * This function should be called in child processes.
 * file_name: The name of the file to analyze.
 * out_fd: The file descriptor to which results are written
 * Returns 0 on success or -1 on error
 */
int process_file(const char *file_name, int out_fd) {
    // Array of known size -> using static allocation
    int counts[26] = {0};

    // TODO
    if (count_letters(file_name, counts) == -1) {
        /* Error printing handled by count_letters */
        return -1;
    }

    // Write to file descriptor atomically
    if (write(out_fd, counts, sizeof(int) * ALPHABET_LEN) == -1) {
        perror("write");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }

    // Create a pipe for child processes to write their results
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");    // errno set
        return 1;
    }

    // Fork a child to analyze each specified file (names are argv[1], argv[2], ...)
    // there are argc-1 children in total
    for (int i = 1; i < argc; i++) {
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");    // errno set
            // Stop reading and writing to pipe
            close(fds[0]);
            close(fds[1]);

            // Wait until all already existing children die
            for (int j = 1; j < i; j++) {
                wait(NULL);
            }
            return 1;
        } else if (child_pid == 0) {
            /* Child process */
            close(fds[0]);    // Stop reading from child

            // Write character counts of file to pipe
            if (process_file(argv[i], fds[1]) == -1) {
                /* Error printing handled by process_file */
                close(fds[1]);    // Stop writing from child
                return 1;
            }

            close(fds[1]);    // Stop writing from child

            // Terminate child process with success code
            return 0;
        }
    }

    close(fds[1]);    // Stop writing from parent

    // Wait for all the children to finish
    for (int i = 1; i < argc; i++) {
        if (wait(NULL) == -1) {
            perror("wait");
            close(fds[0]);    // Stop reading from parent
            return 1;
        }
    }

    // Arrays to compute total letter counts from all files
    int total[ALPHABET_LEN] = {0};
    int letters[ALPHABET_LEN] = {0};

    // Aggregate all the results together by reading from the pipe in the parent
    int nbytes;
    while ((nbytes = read(fds[0], letters, sizeof(int) * ALPHABET_LEN)) > 0) {
        for (int i = 0; i < ALPHABET_LEN; i++) {
            total[i] += letters[i];
        }
    }

    close(fds[0]);

    if (nbytes == -1) {
        perror("read");
        return 1;
    }

    // Print out the total count of each letter (case insensitive)
    for (int i = 0; i < ALPHABET_LEN; i++) {
        printf("%c Count: %d\n", 'a' + i, total[i]);
    }
    return 0;
}
