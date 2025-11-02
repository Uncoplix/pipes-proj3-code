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
    FILE *fh = fopen(file_name, "r");
    if (fh == NULL) {
        perror("fopen");
        return -1;
    }

    char letter;
    while (fread(&letter, 1, sizeof(char), fh) > 0) {
        if (!isalpha(letter)) {
            continue;
        }
        char lower = tolower(letter);    // Letter is in alphabet, make it lowercase.
        counts[lower % 'a'] += 1;
    }

    if (ferror(fh)) {
        perror("fread");
        fclose(fh);
        return -1;
    }

    if (fclose(fh) == EOF) {
        perror("fclose");
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

    if (count_letters(file_name, counts) == -1) {
        fprintf(stderr, "count_letters()\n");
        return -1;
    }

    // Write to file descriptor atomically
    if (write(out_fd, counts, sizeof(int) * 26) == -1) {
        fprintf(stderr, "write()\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }

    // TODO Create a pipe for child processes to write their results
    // TODO Fork a child to analyze each specified file (names are argv[1], argv[2], ...)
    // TODO Aggregate all the results together by reading from the pipe in the parent

    // TODO Change this code to print out the total count of each letter (case insensitive)
    for (int i = 0; i < ALPHABET_LEN; i++) {
        printf("%c Count: %d\n", 'a' + i, -1);
    }
    return 0;
}
