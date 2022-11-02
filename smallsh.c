#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// macros
#define MAX_ARGS 512
#define PATH_MAX 4096

// definition of the command struct
struct command{
  char* cmd;
  char* args[MAX_ARGS];
  char* input_file;
  char* output_file;
  bool background;
  bool comment;
  bool empty;
};

typedef struct command CMD;

// Function declarations
void parse_cmd(CMD *new_cmd);
void print_cmd(CMD *new_cmd);
void free_cmd(CMD *new_cmd);
char *pid_expansion(char *base);

// main method of the program, control flow
int main(int argc, char *argv[]) {

  // initialize the command struct
  CMD new_cmd;

  // initialize the built in variables
  char *current_path;
  char *new_path;
  int dir_status = 0;
  
  // initialize exit status
  int exit_status = 0;
  
  while (true) {

    printf(": ");
    parse_cmd(&new_cmd);
    print_cmd(&new_cmd);

    /* Here's where the magic happens. */
    // if the first char in cmd is a comment, we do nothing
    // if the cmd struct is blank, so cmd is null, we do nothing
    if (!new_cmd.comment && !new_cmd.empty) { 
        // exit
        if (strcmp(new_cmd.cmd, "exit") == 0) {
          printf("hey nice exit\n");
        }
        // status
        else if (strcmp(new_cmd.cmd, "status") == 0) {
          printf("hey nice status\n");
        }
        // cd
        else if (strcmp(new_cmd.cmd, "cd") == 0) {
          printf("hey nice cd\n");

          // check for change to home directory
          if (new_cmd.args[1] == NULL) {
            printf("switch to home\n");
            new_path = getenv("HOME");
            if (chdir(new_path) == -1) perror("cd home");
          // check for an absolute path
          } else if (new_cmd.args[1][0] == '/') {
            printf("absolute path\n");
            if (chdir(new_cmd.args[1]) == -1) perror("cd absolute path");
          // relative path
          } else {
           printf("relative path\n");
           current_path = getcwd(NULL, PATH_MAX);
           new_path = calloc(strlen(current_path) + strlen(new_cmd.args[1]) + 2, 1);
           strcpy(new_path, current_path);
           strcat(new_path, "/");
           strcat(new_path, new_cmd.args[1]);
           printf("%s\n", new_path);
           if (chdir(new_path) == -1) perror("cd relative path");
           free(current_path);
           free(new_path);
          }
        }
        // all other commands
        else {
          printf("you want something else? sure!\n");
          //fork
          ////switch case
          ///child exec
          ///parent wait
          ///error
        }
      }

    free_cmd(&new_cmd);
    printf("\n");
  }

  return EXIT_SUCCESS;
}

void parse_cmd(CMD *new_cmd) {

  char *buffer = NULL;
  size_t num_bytes;
  ssize_t input_len;
  char *tok_ptr;
  char *delim = " ";
  size_t tok_len;
  errno = 0;
  bool cmd_flag = true;
  bool input_flag = false;
  bool output_flag = false;
  int arg_i = 1;

  char *str_ptr;
  // initialize the fields that may not take on another value
  new_cmd->cmd = NULL;
  for (size_t i = 0; i < MAX_ARGS; i++) new_cmd->args[i] = NULL;
  new_cmd->input_file = NULL;
  new_cmd->output_file = NULL;
  new_cmd->background = false;

  // read the line into the buffer using getline()
  // this will automatically allocate this buffer, which will need to be freed.
  // so this pointer needs to be stored in struct, so it can later be freed.
  input_len = getline(&buffer, &num_bytes, stdin);

  // perform the check for an empty line or a comment line here.
  if (buffer[0] == '\n') {
    printf("empty command\n");
    new_cmd->empty = true;
    goto exit;
  } else if (buffer[0] == '#') {
    printf("comment\n");
    new_cmd->comment = true;
    goto exit;
  }

  // remove the trailing newline character
  buffer[input_len - 1] = '\0';

  /* Time to parse input! */
  tok_ptr = strtok(buffer, delim);

    do {

      /* Check for instances where we do not need to store a string, but instead need to set a flag: */
      if (strcmp(tok_ptr, "<") == 0) {
        input_flag = true;
      } else if (strcmp(tok_ptr, ">") == 0) {
        output_flag = true;
      } else if (strcmp(tok_ptr, "&") == 0) {
        new_cmd->background = true;
      } else {
      
        /* Holds everything where tok_ptr -> str_ptr and stored */
                
        tok_len = strlen(tok_ptr);
        str_ptr = malloc(tok_len + 1);
        strcpy(str_ptr, tok_ptr);
        str_ptr = pid_expansion(str_ptr);

        if (cmd_flag) {
          new_cmd->cmd = str_ptr;
          new_cmd->args[0] = str_ptr;
          cmd_flag = false;
        } else if (input_flag) {
          new_cmd->input_file = str_ptr;
          input_flag = false;
        } else if (output_flag) {
          new_cmd->output_file = str_ptr;
          output_flag = false;
        } else {
          new_cmd->args[arg_i] = str_ptr;
          ++arg_i;
        }

      }

      tok_ptr = strtok(NULL, delim);

    } while (tok_ptr != NULL);


exit:
    free(buffer);
    return;
}

void free_cmd(CMD *new_cmd) {

  // overwrite the command
  if (new_cmd->cmd != NULL) {
    free(new_cmd->cmd);
    new_cmd->cmd = NULL;
  }

  // overwrite the args
  int i = 1;
  while (new_cmd->args[i] != NULL) {
    free(new_cmd->args[i]);
    new_cmd->args[i] = NULL;
    ++i;
  }

  // overwrite input file
  if (new_cmd->input_file != NULL) {
    free(new_cmd->input_file);
    new_cmd->input_file = NULL;
  }

  // overwrite output file
  if (new_cmd->output_file != NULL) {
    free(new_cmd->output_file);
    new_cmd->output_file = NULL;
  }

  // set background to false
  new_cmd->background = false;
  new_cmd->comment = false;
  new_cmd->empty = false;

  return;
}

void print_cmd(CMD *new_cmd) {
   
  printf("cmd: %s\n", new_cmd->cmd);

  int i = 0;
  while (new_cmd->args[i] != NULL) {
    printf("arg[%d]: %s\n", i, new_cmd->args[i]);
    ++i;
  }

  if (new_cmd->input_file != NULL) {
    printf("input file: %s\n", new_cmd->input_file);
  }

  if (new_cmd->output_file != NULL) {
    printf("output file: %s\n", new_cmd->output_file);
  }

  if (new_cmd->background) {
    printf("running in the background\n");
  }

  return;
}

char *pid_expansion(char *base) {

  int occurrences = 0;

  // get the pid number
  pid_t pid_num = getpid();

  // cast our pid number into a string for the string concatenation
  char *pid_str = (char *)malloc(sizeof(long));
  sprintf(pid_str, "%d", pid_num);

  // search for occurrences of substring "$$"
  int i = 0;
  while( i < strlen(base)) {
  // look for occurrences
    if (strncmp(&base[i], "$$", 2) == 0) {
      ++occurrences;
      i = i + 2;
    } else {
      ++i;
    }
  }

  if (occurrences != 0) {
    size_t new_len = strlen(base) + occurrences*(strlen(pid_str) - 2) + 1;
    char *new_str = calloc(new_len, new_len);

    // fill in the new string!
    // i in the base string, while i is not at the end, scan for occurrences.
    // so, at each i, if it does not equal a $, copy char into new str
    // otherwise, if i $ and i+1 $, concatenate pid, then move i forward by 2
    i = 0;
    while (i < strlen(base)) {
      // look for occurrences of $$
      if (strncmp(&base[i], "$$", 2) == 0) {
        strcat(new_str, pid_str);
        i = i + 2;
      } else {
        strncat(new_str, &base[i], 1);
        ++i;
      }
    }

    free(base);
    free(pid_str);
    return new_str;
     
  }

  return base;
}

