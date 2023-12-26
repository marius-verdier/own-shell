#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __unix__
    #define IS_POSIX 1
#else
    #define IS_POSIX 0
#endif

/**
 * declaration for builtin shell commands (not definition)
*/
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);

/**
 * list of builtin commands, and their corresponding functions
*/
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "pwd"
};
int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &lsh_pwd
};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char*);
}

/**
 * Builtin functions implementation
*/
int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("lsh");
        }
    }
    return 1;
}
int lsh_help(char **args) 
{
    int i;
    printf("CDR shell, participation pôle banivo\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) 
    {
        printf("  %s\n", builtin_str[i]);
    }

    return 1;
}
int lsh_exit(char **args)
{
    return 0;
}
#define PATH_MAX 1024
int lsh_pwd(char **args)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("If i guess correctly, you're currently working here: %s\n", cwd);
        return 1;
    } else {
        perror("getcwd() error");
        return 1;
    }
    fprintf(stderr, "Oups, I guess something bad happened, let's try again!\n");
    return 1;
    
}


/**
 * To launch a process, you fork to duplicate an existing process
 * and you exec to kill the current process and create a new one
 * 
*/
int lsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork(); // now we have two processes running 
    if (pid == 0) // child process
    {
        /**
         * run the command given
         * execvp : variant of exec
         *  -> v means need a vector as argument (first one is the name of the command)
         *  -> p means no need to path for the command, just the name and the operating
        */
        if (execvp(args[0], args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } 
    else if (pid < 0) // fork problem
    {
        perror("lsh");
    }
    else
    {
        do 
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
    
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize) // same as in read_line, we reallocate when out of bounds
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) 
            {
                free(tokens_backup);
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

#define LSH_RL_BUFSIZE 1024 // size before reallocate in memory
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar(); // read character
        
        //EOF : end of stream -> if we hit end of line / stream, we replace the last char by null
        if (c == EOF) {
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // if buffer excedeed, we reallocate
        if (position >= bufsize) 
        {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if(!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int lsh_execute(char **args)
{
    int i;

    if(args[0] == NULL)
    {
        return 1;
    }

    for (i = 0 ; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

void lsh_loop(void) 
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) 
{
    // Config files

    lsh_loop();

    // Shutdown, cleanup

    return EXIT_SUCCESS;
}