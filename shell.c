#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/*
    In-Built functions
*/ 
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = { // array of strings
    "cd",
    "help",
    "exit"
};
int (*builtin_func[]) (char**) = { // array of function pointers that return int and take array of strings as arg
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char*); // would not wish this on my worst enemies
}

int lsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"n");
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char **args) {
    int i;
    printf("Kaveri's LSH\n");
    printf("Type program names and arguments and press enter.\n");
    printf("The following functions are built-in: \n\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args) {
    return 0;
}

/*
    Read line from input stream
*/

char *lsh_read_line(void) {
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(bufsize);
    int c;

    if (buffer == NULL) {
        fprintf(stderr, "lsh: read buffer allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0'; // null terminated array of chars, so '\0'
            return buffer;
        }
        else {
            buffer[position] = c;
            position++;
        }

        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (buffer == NULL) {
                fprintf(stderr, "lsh: read buffer reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
    Split line into program name and args
*/

char **lsh_split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (tokens == NULL) {
        fprintf(stderr, "lsh: tokenize buffer allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (tokens == NULL) {
                fprintf(stderr, "lsh: tokenize buffer reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL; // null terminated array of pointers of chars, so NULL

    return tokens;
}

/*
    Launch child process with exec
*/

int lsh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) { // child
        if (execvp(args[0], args) == -1) { // error in exec
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        perror("lsh"); // error in fork
    }
    else { // parent
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // nani??
    }

    return 1;
}

/*
    Launch either built-in function or child process
*/
int lsh_execute(char **args) {
    if (args[0] == NULL) return 1;

    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) { // arg[0] is a built-in fn
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

/*
    Main loop
*/

void lsh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    }
    while (status);
} 

int main() {
    lsh_loop();
    return 0;
}
