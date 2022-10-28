#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// macros
#define MAX_ARGS 512

// definition of the command struct
struct command{
  char* cmd;
  char* args[MAX_ARGS];
  char* input_file;
  char* output_file;
  bool background;
};

typedef struct command CMD;

// Function declarations
void parse_cmd(CMD *new_cmd);
void print_cmd(CMD *new_cmd);
void free_cmd(CMD *new_cmd);

// main method of the program, control flow
int main(int argc, char *argv[]) {

  // initialize the command struct
  CMD new_cmd;
  CMD *cmd_ptr = &new_cmd;
  for (size_t i = 0; i < MAX_ARGS; i++) new_cmd.args[i] = NULL;

    printf(": ");
    parse_cmd(cmd_ptr);
    print_cmd(cmd_ptr);
    free_cmd(cmd_ptr);

  /* Develop code to free all memory allocated to the struct
   * this will also become its own function. one day. i believe in it. */

  return EXIT_SUCCESS;
}

void parse_cmd(CMD *new_cmd) {
 
  char *ptr;
  int n = 0;
  errno = 0;
  int nl_char;

  /* parsing should be in its own function and the struct returned 
   * for now, here is the code to parse */
  bool cmd_flag = true;
  bool input_flag = false;
  bool output_flag = false;
  int arg_i = 0;

  // initialize the fields that may not take on another value
  new_cmd->input_file = NULL;
  new_cmd->output_file = NULL;
  new_cmd->background = false;

    /* Time to parse input! */
    do {
      n = scanf("%ms", &ptr);

      if (errno != 0) {
        perror("scanf");
      } else {
        if (cmd_flag) {
          new_cmd->cmd = ptr;
          cmd_flag = false;
        } else if (input_flag) {
          new_cmd->input_file = ptr;
          input_flag = false;
        } else if (output_flag) {
          new_cmd->output_file = ptr;
          output_flag = false;
        } else {
          // no specific flag. check if the string is > or < of & otherwise save as next arg
          if (strcmp(ptr, "<") == 0) {
            input_flag = true;
            free(ptr);
          } else if (strcmp(ptr, ">") == 0) {
            output_flag = true;
            free(ptr);
          } else if (strcmp(ptr, "&") == 0) {
            new_cmd->background = true;
            free(ptr);
          } else {
            new_cmd->args[arg_i] = ptr;
            ++arg_i;
          }
        }
      }

      // check if we have hit the new line, if so break
      nl_char = getchar();
      if (nl_char == '\n') {
          printf("\n");
          break;
      }

    } while (n != 0);

  return;
}

void free_cmd(CMD *new_cmd) {
  // free command
  free(new_cmd->cmd);

  //free args loop
  int i = 0;
  while (new_cmd->args[i] != NULL) {
    free(new_cmd->args[i]);
    new_cmd->args[i] = NULL;
    ++i;
  }

  // free input, output
  if (new_cmd->input_file != NULL) {
    free(new_cmd->input_file);
    new_cmd->input_file = NULL;
  }

  if (new_cmd->output_file != NULL) {
    free(new_cmd->output_file);
    new_cmd->output_file = NULL;
  }

  // set background to false
  new_cmd->background = false;

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

