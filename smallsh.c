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
  bool comment;
  bool empty;
};

typedef struct command CMD;

// Function declarations
void parse_cmd(CMD *new_cmd);
void print_cmd(CMD *new_cmd);
void free_cmd(CMD *new_cmd);
void pid_expansion(CMD *new_cmd);

// main method of the program, control flow
int main(int argc, char *argv[]) {

  // initialize the command struct
  CMD new_cmd;
  
  while (true) {

    printf(": ");
    parse_cmd(&new_cmd);
    print_cmd(&new_cmd);
    pid_expansion(&new_cmd);
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
  int arg_i = 0;

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

        if (cmd_flag) {
          new_cmd->cmd = str_ptr;
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
  int i = 0;
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

void pid_expansion(CMD *new_cmd) {
  /* The idea here is to scan through every string inside of the cmd struct. If we find
   * an instance of $$ in a row, we want to replace that. So set an expansion flag, count
   * the number of occurrences. malloc a new string, create the new expanded string, free
   * the old string, then have the struct point to our new string.
   * Easy.*/

  bool expand = false;
  int occurrences = 0;

  // check new_cmd->cmd
  for(int i = 0; i<strlen(new_cmd->cmd); i++) {
    if (strncmp(&new_cmd->cmd[i], "$$", 2) == 0) {
      bool expand = true;
      ++occurrences;
      printf("found an expansion!");
    }
  }
  //
  // check new_cmd->args
  //
  // check new_cmd->input_file
  //
  // check new_cmd->output_file


  return;
}

