
#include <sys/wait.h> // waitpid()
#include <sys/types.h> 
#include <unistd.h>  // chdir(), fork(), exec(), pid_t()
#include <stdlib.h> // malloc(), realloc(), free(), exit(), execvp(), EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h> // fprintf(), printf(), stderr, getchar(), perror()
#include <string.h> // strcmp(), strtok()

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

int lsh_cd(char **args)
{
  if (args[1] == NULL) { // no directory argument was provided 
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) { // chdir() -> -1 if failure to change directory
      perror("lsh"); // return error
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Dior's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) { // print list of built-in commands
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_launch(char **args)
{
  pid_t pid; // stores process ID of child process 
  int status; // stores exit status of child process once it terminates

  pid = fork(); // fork new process
  if (pid == 0) { // if current process is the child process, execute command with execvp()
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) { // if 'fork' system call fails and no child process was created, error message printed
    perror("lsh");
  } else { // curent process is the parent process
    do {
      waitpid(pid, &status, WUNTRACED); // wait for the child process to complete
    } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // once completed, loop terminates
  }

  return 1;
}

int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) { // check if empty
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) { // check if built in command by iterating through the array of built-in commands
    if (strcmp(args[0], builtin_str[i]) == 0) { // if found, enter command
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE // read line of input from standard input stdin
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else // if LSH_USE_STD_GETLINE not defined, use manual approach to read input 
#define LSH_RL_BUFSIZE 1024 // initial buffer size, dynamically reallocated if full
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize); // dynamic reallocation
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) { // infinite loop to read stdin until EOF encountered, or newline character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS); // exit shell
    } else if (c == '\n') {
      buffer[position] = '\0'; // terminate the line
      return buffer; //returns the buffer
    } else {
      buffer[position] = c; // update buffer
    }
    position++;

    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM); // extracts first token from input line
  while (token != NULL) { // store extracted token in tokens
    tokens[position] = token;
    position++;

    if (position >= bufsize) { // check if current position exceeds buffer size
      bufsize += LSH_TOK_BUFSIZE; // if exceeds, tokens array needs to be resized to accommodate more tokens
      tokens_backup = tokens; // memory reallocation
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) { // if realloc fails
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


void lsh_loop(void)
{
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
  } while (status);
}

int main(int argc, char **argv)
{

  lsh_loop();


  return EXIT_SUCCESS;
}
