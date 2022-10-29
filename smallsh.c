#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// macros
#define MAX_ARGS 512

// definition of the command struct
struct command{
  char* cmd_buffer;
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

// main method of the program, control flow
int main(int argc, char *argv[]) {

  // initialize the command struct
  CMD new_cmd;
  CMD *cmd_ptr = &new_cmd;
  for (size_t i = 0; i < MAX_ARGS; i++) new_cmd.args[i] = NULL;

  while (true) {

    printf(": ");
    parse_cmd(cmd_ptr);
    print_cmd(cmd_ptr);

    /* Here's where the magic happens. */
    // if the first char in cmd is a comment, we do nothing
    // if the cmd struct is blank, so cmd is null, we do nothing
    if (new_cmd.cmd != NULL) {
      if ( strncmp(new_cmd.cmd, "#", 1) != 0 ) {
      
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
    }

    free_cmd(cmd_ptr);
    printf("\n");


  }
  /* Develop code to free all memory allocated to the struct
   * this will also become its own function. one day. i believe in it. */

  return EXIT_SUCCESS;
}

void parse_cmd(CMD *new_cmd) {

  char *buffer = NULL;
  size_t buffer_bytes;
  char *ptr;
  char *delim = " ";
  errno = 0;
  bool cmd_flag = true;
  bool input_flag = false;
  bool output_flag = false;
  int arg_i = 0;

  // initialize the fields that may not take on another value
  new_cmd->cmd = NULL;
  new_cmd->input_file = NULL;
  new_cmd->output_file = NULL;
  new_cmd->background = false;

  // read the line into the buffer using getline()
  // this will automatically allocate this buffer, which will need to be freed.
  // so this pointer needs to be stored in struct, so it can later be freed.
  getline(&buffer, &buffer_bytes, stdin);
  new_cmd->cmd_buffer = buffer;

  // perform the check for an empty line or a comment line here.
  if (buffer[0] == '\n') {
    printf("empty command\n");
    new_cmd->empty = true;
    return;
  } else if (buffer[0] == '#') {
    printf("comment\n");
    new_cmd->comment = true;
    return;
  }


    
  /* Time to parse input! */
  ptr = strtok(buffer, delim);

    do {
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
          } else if (strcmp(ptr, ">") == 0) {
            output_flag = true;
          } else if (strcmp(ptr, "&\n") == 0) {
            new_cmd->background = true;
          } else {
            new_cmd->args[arg_i] = ptr;
            ++arg_i;
          }
        }
      }
      ptr = strtok(NULL, delim);

    } while (ptr != NULL);


exit:
  return;
}

void free_cmd(CMD *new_cmd) {

  // overwrite the command
  if (new_cmd->cmd != NULL) {
    new_cmd->cmd = NULL;
  }

  // overwrite the args
  int i = 0;
  while (new_cmd->args[i] != NULL) {
    new_cmd->args[i] = NULL;
    ++i;
  }

  // overwrite input file
  if (new_cmd->input_file != NULL) {
    new_cmd->input_file = NULL;
  }

  // overwrite output file
  if (new_cmd->output_file != NULL) {
    new_cmd->output_file = NULL;
  }

  // set background to false
  new_cmd->background = false;
  new_cmd->comment = false;
  new_cmd->empty = false;

  free(new_cmd->cmd_buffer);

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

