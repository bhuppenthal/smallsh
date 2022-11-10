/*
 * Brenda Huppenthal
 * CS344 Operating Systems
 * 11/13/2022
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// macros
#define MAX_ARGS 512
#define PATH_MAX 4096
#define DA_INIT_LEN 5

// definition of the command struct
struct command {
  char *args[MAX_ARGS];
  char *input_file;
  char *output_file;
  bool background;
  bool comment;
  bool empty;
};

typedef struct command CMD;

// definition of the dynamic array struct
struct dynamic_array {
  pid_t *array;
  int length;
  int num_elem;
};

typedef struct dynamic_array DA;

// Function declarations
void parse_cmd(CMD *new_cmd);
void print_cmd(CMD *new_cmd);
void free_cmd(CMD *new_cmd);
char *pid_expansion(char *base);
void exec_cd(char *args[]);
void da_append(DA *da, pid_t new_elem);
void da_remove(DA *da, int index);
void da_print(DA *da);
void redirect(CMD *cmd);
void kill_children(DA *da);
void check_children(DA *da);

// main method of the program, control flow
int main(int argc, char *argv[]) {
  /*
   * Main method.
   * ------------------------------
   * Controls the program flow. Has the main loop for accepting input from the user.
   *
   * Parameters:
   *  argc: the number of arguments
   *  argv: the arguments as strings. argv[0] is always the program name, if there were more args.
   *
   *  Returns:
   *   Exit 0 on success.
   */

  CMD cmd;

  DA bg_pids;
  bg_pids.array = calloc(DA_INIT_LEN, sizeof(pid_t));
  bg_pids.length = DA_INIT_LEN;
  bg_pids.num_elem = 0;
 
  int exit_status = 0;
  int exit_signal = 0;

  pid_t child_pid = -100;
  int child_status;
  int pid_status;


  while (true) {

    printf(": ");
    fflush(NULL);
    parse_cmd(&cmd);

    /* Here's where the magic happens. */
    if (!cmd.comment && !cmd.empty) {
      if (strcmp(cmd.args[0], "exit") == 0) {
        // this will kill any child processes
        goto exit;
      }

      else if (strcmp(cmd.args[0], "status") == 0) {
        printf("exit value %d\n", exit_status);
        fflush(NULL);
      }

      else if (strcmp(cmd.args[0], "cd") == 0) {
        exec_cd(cmd.args);
      }

      else {
        child_pid = fork();

        switch (child_pid) {
        // error
        case -1:
          perror("fork() failed");
          fflush(NULL);
          exit(1);
          break;

        // child process block
        case 0:
          redirect(&cmd);
          execvp(cmd.args[0], cmd.args);
          perror("execvp");
          fflush(NULL);
          exit(1);
          break;

        // parent process block
        default:
          // background process
          if (cmd.background) {
            printf("background pid is %d\n", child_pid);
            fflush(NULL);
            da_append(&bg_pids, child_pid);
            child_pid = waitpid(child_pid, &child_status, WNOHANG); // probably need to change this in case it
                                  // ends this quick, do we even need this line?
          }
          // foreground process
          else {
            child_pid = waitpid(child_pid, &child_status, 0);
            if (WIFEXITED(child_status)) {
              exit_status = WEXITSTATUS(child_status);
            } else {
              exit_signal = WTERMSIG(child_status);
            }
          }
          break;
        }
      }
    }
    // before returning control to the user,
    // check if any of the background processes have terminated
    check_children(&bg_pids); 
    free_cmd(&cmd);
  }

exit:
  free_cmd(&cmd);
  free(bg_pids.array);
  return EXIT_SUCCESS;
}

void parse_cmd(CMD *cmd) {

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
  for (size_t i = 0; i < MAX_ARGS; i++)
    cmd->args[i] = NULL;
  cmd->input_file = NULL;
  cmd->output_file = NULL;
  cmd->background = false;

  // read the line into the buffer using getline()
  // this will automatically allocate this buffer, which will need to be freed.
  // so this pointer needs to be stored in struct, so it can later be freed.
  input_len = getline(&buffer, &num_bytes, stdin);

  // perform the check for an empty line or a comment line here.
  if (buffer[0] == '\n') {
    cmd->empty = true;
    goto exit;
  } else if (buffer[0] == '#') {
    cmd->comment = true;
    goto exit;
  }

  // remove the trailing newline character
  buffer[input_len - 1] = '\0';

  /* Time to parse input! */
  tok_ptr = strtok(buffer, delim);

  do {

    /* Check for instances where we do not need to store a string, but instead
     * need to set a flag: */
    if (strcmp(tok_ptr, "<") == 0) {
      input_flag = true;
    } else if (strcmp(tok_ptr, ">") == 0) {
      output_flag = true;
    } else if (strcmp(tok_ptr, "&") == 0) {
      cmd->background = true;
    } else {

      /* Holds everything where tok_ptr -> str_ptr and stored */

      tok_len = strlen(tok_ptr);
      str_ptr = malloc(tok_len + 1);
      strcpy(str_ptr, tok_ptr);
      str_ptr = pid_expansion(str_ptr);

      if (cmd_flag) {
        cmd->args[0] = str_ptr;
        cmd_flag = false;
      } else if (input_flag) {
        cmd->input_file = str_ptr;
        input_flag = false;
      } else if (output_flag) {
        cmd->output_file = str_ptr;
        output_flag = false;
      } else {
        cmd->args[arg_i] = str_ptr;
        ++arg_i;
      }
    }

    tok_ptr = strtok(NULL, delim);

  } while (tok_ptr != NULL);

exit:
  free(buffer);
  return;
}

void free_cmd(CMD *cmd) {

  // overwrite the args
  int i = 0;
  while (cmd->args[i] != NULL) {
    free(cmd->args[i]);
    cmd->args[i] = NULL;
    ++i;
  }

  // overwrite input file
  if (cmd->input_file != NULL) {
    free(cmd->input_file);
    cmd->input_file = NULL;
  }

  // overwrite output file
  if (cmd->output_file != NULL) {
    free(cmd->output_file);
    cmd->output_file = NULL;
  }

  // set background to false
  cmd->background = false;
  cmd->comment = false;
  cmd->empty = false;

  return;
}

void print_cmd(CMD *cmd) {

  int i = 0;
  while (cmd->args[i] != NULL) {
    printf("arg[%d]: %s\n", i, cmd->args[i]);
    ++i;
  }

  if (cmd->input_file != NULL) {
    printf("input file: %s\n", cmd->input_file);
  }

  if (cmd->output_file != NULL) {
    printf("output file: %s\n", cmd->output_file);
  }

  if (cmd->background) {
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
  while (i < strlen(base)) {
    // look for occurrences
    if (strncmp(&base[i], "$$", 2) == 0) {
      ++occurrences;
      i = i + 2;
    } else {
      ++i;
    }
  }

  if (occurrences != 0) {
    size_t new_len = strlen(base) + occurrences * (strlen(pid_str) - 2) + 1;
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
  free(pid_str);
  return base;
}

void exec_cd(char *args[]) {

  // initialize the built in variables
  char *current_path;
  char *new_path;

  // check for change to home directory
  if (args[1] == NULL) {
    new_path = getenv("HOME");
    if (chdir(new_path) == -1)
      perror("cd home");
    // check for an absolute path
  } else if (args[1][0] == '/') {
    printf("absolute path\n");
    if (chdir(args[1]) == -1)
      perror("cd absolute path");
    // relative path
  } else {
    current_path = getcwd(NULL, PATH_MAX);
    new_path = calloc(strlen(current_path) + strlen(args[1]) + 2, 1);
    strcpy(new_path, current_path);
    strcat(new_path, "/");
    strcat(new_path, args[1]);
    if (chdir(new_path) == -1)
      perror("cd relative path");
    free(current_path);
    free(new_path);
  }

  return;
}

void da_append(DA *da, pid_t new_elem) {

  // if length == num_elements, resize to double using realloc
  if (da->num_elem == da->length) {
    da->length = 2 * da->length;
    da->array = realloc(da->array, da->length * sizeof(pid_t));
  }

  da->array[da->num_elem] = new_elem;
  ++da->num_elem;

  return;
}

void da_remove(DA *da, int index) {

  // starting at the index, move elements left until reaching an element of 0
  da->array[index] = 0;

  int i = index + 1;

  while (da->array[i] != 0) {
    da->array[i - 1] = da->array[i];
    ++i;
  }

  --da->num_elem;

  return;
}

void da_print(DA *da) {

  printf("length: %d\nnum_elem: %d\n", da->length, da->num_elem);

  for (int i = 0; i < da->length; ++i) {
    printf("index %d: %d\n", i, da->array[i]);
  }

  return;
}

void redirect(CMD *cmd) {

  int src;
  int dest;

  if (cmd->input_file != NULL) {
    src = open(cmd->input_file, O_RDONLY);
    if (src == -1) {
      perror("open input file for read");
      exit(1);
    }
  } else if (cmd->background) {
    src = open("/dev/null", O_RDONLY);
    if (src == -1) {
      perror("open /dev/null for read");
      exit(1);
    }
  }

  if (cmd->output_file != NULL) {
    dest = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest == -1) {
      perror("open output file for write");
      exit(1);
    }
  } else if (cmd->background) {
    dest = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest == -1) {
      perror("open /dev/null for write");
      exit(1);
    }
  }

  // perform redirection
  if (cmd->input_file != NULL || cmd->background) {
    if (dup2(src, 0) == -1) {
      perror("dup2 input");
      exit(1);
    }
  }
  if (cmd->output_file != NULL || cmd->background) {
    if (dup2(dest, 1) == -1) {
      perror("dup2 output");
      exit(1);
    }
  }

  return;
}

void kill_children(DA *da){

  return;
}

void check_children(DA *da){

  int pid_status;
  int child_status;
  int exit_status;
  int exit_signal;
  
  for (int i = 0; i < da->num_elem; ++i) {
      pid_status = waitpid(da->array[i], &child_status, WNOHANG);

      if (pid_status == -1)
        perror("waitpid on child process");
      else if (pid_status != 0) {

        if (WIFEXITED(child_status)) {
          exit_status = WEXITSTATUS(child_status);
          printf("background pid %d is done: exit value %d\n", pid_status, exit_status);
          fflush(NULL);
        } else {
          exit_signal = WTERMSIG(child_status);
          printf("terminated by signal %d\n", exit_signal);
          fflush(NULL);
        }

        da_remove(da, i);
        --i;
      }
    }


  return;
}
