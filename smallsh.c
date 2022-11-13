/*
 * Brenda Huppenthal
 * CS344 Operating Systems
 * 11/13/2022
 */
#define _POSIX_C_SOURCE 200809L
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
#include <signal.h>

// macros
#define MAX_ARGS 512
#define PATH_MAX 4096
#define DA_INIT_LEN 5

/* Definition of the command struct.
   The argument and redirection strings are stored in allocated memory on the heap. This struct
   stores pointers to their location. */
struct command {
  char *args[MAX_ARGS];
  char *input_file;
  char *output_file;
  bool bg;
  bool comment;
  bool empty;
};

typedef struct command CMD;

/* Definition of the dynamic array struct.
 * This is not a full dynamic array implementation. It will double in size if it needs to store more
 * elements than it has space for, but there is no check to downsize the array.
 * The array of elements is allocated on the heap. A pointer to the first element of the array is stored
 * in this struct. */
struct dynamic_array {
  pid_t *array;
  int length;
  int num_elem;
};

typedef struct dynamic_array DA;

static bool fg_mode = false;

// Function declarations
void cmd_init(CMD *new_cmd);
void cmd_parse(CMD *new_cmd);
void cmd_print(CMD *new_cmd);
void cmd_free(CMD *new_cmd);
char *pid_expansion(char *base);
void exec_cd(char *args[]);
void da_append(DA *da, pid_t new_elem);
void da_remove(DA *da, int index);
void da_print(DA *da);
void redirect(CMD *cmd);
void kill_children(DA *da);
void check_children(DA *da);
void toggle_fg_mode();


int main(void) {
  /* Controls the program flow. Has the main loop for accepting input from the user.
   *
   *  Returns:
   *   Exit 0 on success. */

  /* Holds the user input. */
  CMD cmd;
  cmd_init(&cmd);

  /* Partial implementation of a dynamic array to track the running background PIDs.
   * This will only ever double in size, and will never check if it needs to size down.
   * The array is allocated on the heap, and a pointer to it is stored on the stack in
   * the struct. */
  DA bg_pids;
  bg_pids.array = calloc(DA_INIT_LEN, sizeof(pid_t));
  bg_pids.length = DA_INIT_LEN;
  bg_pids.num_elem = 0;
 
  /* Tracks the state of the last foreground process. */
  int exit_status = 0;
  int exit_signal = 0;

  /* Used to track the foreground and background processes. */
  pid_t child_pid = -100;
  int child_status;

  /* Register a handling funtion to ignore SIGINT. */
  struct sigaction ignore_interrupt; 
  struct sigaction allow_interrupt;
  ignore_interrupt.sa_handler = SIG_IGN;
  sigaction(SIGINT, &ignore_interrupt, &allow_interrupt);

  /* Register a foreground mode handling function to SIGTSTP, which will toggle fg
   * mode on and off when SIGTSTP is delivered. */
  struct sigaction fg_action;
  fg_action.sa_handler = toggle_fg_mode;
  if (sigaddset(&fg_action.sa_mask, SIGTSTP) == -1) perror("sigaddset");
  fg_action.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &fg_action, NULL);

  /*Create a mask to allow the process to block and unblock SIGTSTP. */
  sigset_t mask_sigtstp;
  if (sigaddset(&mask_sigtstp, SIGTSTP) == -1) perror("create mask");


  /* This while loop governs the process of our small shell:
   * 1) Prints the prompt.
   * 2) Unblocks SIGSTSP, allowing any pending signals to be delivered and handled.
   * 3) Accepts, parses, and performs expansion on user input.
   * 4) Blocks SIGTSTP, allowing the foreground and background commands to ignore SIGTSTP.
   * 5) Performs any built in commands.
   * 6) Forks a child process to handle all other commands.
   * 7) Checks all background processes to see if any have completed.
   * 8) Empties the command structure to prepare for the next input. */
  while (true) {

    printf(": ");
    fflush(NULL);

    /* We want to unblock SIGTSTP to allow delivery of pending SIGTSTPs to handle here.
     * After accepting input from the user, block SIGTSTP again. */
    sigprocmask(SIG_UNBLOCK, &mask_sigtstp, NULL);
    cmd_parse(&cmd);
    sigprocmask(SIG_BLOCK, &mask_sigtstp, NULL);

    
    /* Built in functions. */
    if (!cmd.comment && !cmd.empty) {
      if (strcmp(cmd.args[0], "exit") == 0) {
        kill_children(&bg_pids);
        goto exit;
      }
      else if (strcmp(cmd.args[0], "status") == 0) {
        printf("exit value %d\n", exit_status);
        fflush(NULL);
      }
      else if (strcmp(cmd.args[0], "cd") == 0) {
        exec_cd(cmd.args);
      }

      /* Otherwise, fork a child process to handle all other commands with execvp. */
      else {
        child_pid = fork();

        /* Switch case to allow for different behavior for the parent and child processes. */
        switch (child_pid) {

        /* Fork error. */
        case -1:
          perror("fork() failed");
          fflush(NULL);
          exit(1);
          break;

        /* This case is executed by the child. */
        case 0:
          // allow foreground processes to be interrupted by SIGINT 
          if (!cmd.bg || fg_mode) sigaction(SIGINT, &allow_interrupt, NULL);

          // perform redirection of stdin and stdout
          redirect(&cmd);

          /* Execute the command. This function only re*/
          execvp(cmd.args[0], cmd.args);
          perror("execvp");
          fflush(NULL);
          exit(1);
          break;

        /* This case is executed by the parent. */
        default:

          /* Background command, add the child pid to the background pid array. */
          if (cmd.bg && !fg_mode) {
            printf("background pid is %d\n", child_pid);
            fflush(NULL);
            da_append(&bg_pids, child_pid);
          }

          /* Foreground command, wait until completion, and then print out a message. */
          else {
            child_pid = waitpid(child_pid, &child_status, 0);
            if (WIFEXITED(child_status)) {
              exit_status = WEXITSTATUS(child_status);
            } else {
              exit_signal = WTERMSIG(child_status);
              printf("foreground pid %d terminated by signal %d\n", child_pid, exit_signal);
              fflush(NULL);
            }
          }
          break;
        }
      }
    }
    check_children(&bg_pids); 
    cmd_free(&cmd);
  }

/* Before exiting the process, we need to free all allocated memory associated with 
 * the command struct and background PID array. */
exit:
  cmd_free(&cmd);
  free(bg_pids.array);
  return EXIT_SUCCESS;
}

void cmd_init(CMD *cmd) {
  /* Initializes the command struct.
   *
   * Parameters:
   *   *cmd: pointer to the command struct */

  for (size_t i = 0; i < MAX_ARGS; i++) cmd->args[i] = NULL;
  cmd->input_file = NULL;
  cmd->output_file = NULL;
  cmd->bg = false;

  return;
}

void cmd_parse(CMD *cmd) {
  /* Allocates memory for and parses user input.
   * All strings are allocated on the heap, and the command struct stores pointers to their location.
   * Meant to be used with the free_cmd function, which will free allocated storage on the heap.
   *
   * Parameters:
   *    *cmd: pointer to the command struct to store the parsed user input */

  /* Raw user input */
  char *buffer = NULL;
  size_t num_bytes;
  ssize_t input_len = -1;

  /* Tokenization */
  char *tok_ptr;
  char *delim = " ";
  size_t tok_len;
  errno = 0;

  /* Control flags */
  bool input_flag = false;
  bool output_flag = false;
  int arg_i = 0;

  /* Pointer to hold address of string allocation on the heap */
  char *str_ptr;
  
  /* getline allocates memory for the raw user input on the heap */
  input_len = getline(&buffer, &num_bytes, stdin);

  /* If the command is empty or is a comment, disregard remaining contents of buffer */
  if (buffer[0] == '\n') {
    cmd->empty = true;
    goto exit;
  } else if (buffer[0] == '#') {
    cmd->comment = true;
    goto exit;
  }

  /* Trim the trailing newline character */
  buffer[input_len - 1] = '\0';

  /* Time to parse input! This loops until we have processed all space delimited tokens. */
  tok_ptr = strtok(buffer, delim);
  do {

    /* Check for characters that indicate file redirection or background status */
    if (strcmp(tok_ptr, "<") == 0) {
      input_flag = true;
    } else if (strcmp(tok_ptr, ">") == 0) {
      output_flag = true;
    } else if (strcmp(tok_ptr, "&") == 0 && tok_ptr == (buffer + input_len - 2)){
      cmd->bg = true;
    } else {

      /* Allocates space on the heap to store the token.
       * I am allocating a new location for the token because I want to free the buffer at the
       * end of this function, which could cause any tokens I still had in the buffer to be
       * overwritten by future allocations on the heap. */
      tok_len = strlen(tok_ptr);
      str_ptr = malloc(tok_len + 1);
      strcpy(str_ptr, tok_ptr);
      str_ptr = pid_expansion(str_ptr);

      /* Check control flags and store the token appropriately. */
      if (input_flag) {
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

/* Frees the buffer as all tokens have been stored elsewhere and it will not be used again. */
exit:
  free(buffer);
  return;
}

void cmd_free(CMD *cmd) {
  /* Frees all memory used by the previous command and readies the command struct for the 
   * next user input.
   *
   * Parameters:
   *   *cmd: pointer to the command struct that stores user input */

  int i = 0;
  while (cmd->args[i] != NULL) {
    free(cmd->args[i]);
    cmd->args[i] = NULL;
    ++i;
  }

  if (cmd->input_file != NULL) {
    free(cmd->input_file);
    cmd->input_file = NULL;
  }

  if (cmd->output_file != NULL) {
    free(cmd->output_file);
    cmd->output_file = NULL;
  }

  cmd->bg = false;
  cmd->comment = false;
  cmd->empty = false;

  return;
}

void cmd_print(CMD *cmd) {
  /* Prints the current contents of the command struct for debugging purposes.
   *
   * Parameters:
   *   *cmd: pointer to the command struct that stores user input */
  
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

  if (cmd->bg) {
    printf("running in the background\n");
  }

  return;
}

char *pid_expansion(char *og_str) {
  /* Performs process id expansion of the string. If the string does not have $$, this method returns the same
   * pointer that it was passed. Otherwise, it allocates a new string, performs the expansion, and frees the
   * old string before returning the pointer to the new string.
   *
   * Parameters:
   *   *og_str: the original string to perform string expansion on */

  int occurrences = 0;

  /* Gets the process ID and casts it to a string for string concatenation. */
  pid_t pid_num = getpid();
  char *pid_str = (char *)malloc(sizeof(long));
  sprintf(pid_str, "%d", pid_num);

  /* Counts occurrences of $$. */
  size_t i = 0;
  while (i < strlen(og_str)) {
    if (strncmp(&og_str[i], "$$", 2) == 0) {
      ++occurrences;
      i = i + 2;
    } else {
      ++i;
    }
  }

  if (occurrences != 0) {
    size_t new_len = strlen(og_str) + occurrences * (strlen(pid_str) - 2) + 1;
    char *new_str = calloc(new_len, new_len);

    /* Scans for $$ and if found, concatenates the process ID to the string. */
    i = 0;
    while (i < strlen(og_str)) {
      if (strncmp(&og_str[i], "$$", 2) == 0) {
        strcat(new_str, pid_str);
        i = i + 2;
      } else {
        strncat(new_str, &og_str[i], 1);
        ++i;
      }
    }

    free(og_str);
    free(pid_str);
    return new_str;
  }

  free(pid_str);
  return og_str;
}

void exec_cd(char *args[]) {
  /* Implements the functionality of the built in change directory command using the chdir function.
   *
   * Paramters:
   *   *args[]: argument array governing the command */

  char *new_path;

  /* Changes the directory to the home directory. */
  if (args[1] == NULL) {
    new_path = getenv("HOME");
    if (chdir(new_path) == -1) perror("cd home");
  }

  /* Changes the directory to the absolute or relative path. */
  else {
    if (chdir(args[1]) == -1) perror("cd absolute/relative path");
  }

  return;
}

void da_append(DA *da, pid_t new_elem) {
  /* Appends an element to the dynamic array. If the array is full before the append, this method will double the size of the
   * array using the realloc function.
   *
   * Parameters:
   *   *da: a pointer to the dynamic array struc
   *   new_elem: the element to append to the dynamic array */

  /* Reallocates the array to twice the current size. */
  if (da->num_elem == da->length) {
    da->length = 2 * da->length;
    da->array = realloc(da->array, da->length * sizeof(pid_t));
  }

  da->array[da->num_elem] = new_elem;
  ++da->num_elem;

  return;
}

void da_remove(DA *da, int index) {
  /* Removes the element at index from the array, and moves the remaining elements to fill in the empty space.
   *
   * Parameters:
   *   *da: pointer to the dynamic array struct
   *   index: index of the element to remove */

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
  /* Prints the contents of the dynamic array struct for debugging purposes.
   *
   * Parameters:
   *   *da: pointer to the dynamic array struct */

  printf("length: %d\nnum_elem: %d\n", da->length, da->num_elem);

  for (int i = 0; i < da->length; ++i) {
    printf("index %d: %d\n", i, da->array[i]);
  }

  return;
}

void redirect(CMD *cmd) {
  /* Performs input and output redirection for the child processes.
   *
   * If the input or output has been redirected, performs that redirection. Any background processes without explicit
   * redirection should default to /dev/null. Failure to open a file or redirect results in the child process exiting
   * with an exit status of 1.
   *
   * Parameters:
   *   *cmd: pointer to the command struct */

  int src;
  int dest;

  /* Opens the input file as read only.
   * If a background command has no specified input, opens /dev/null. */
  if (cmd->input_file != NULL) {
    src = open(cmd->input_file, O_RDONLY);
    if (src == -1) {
      perror("open input file for read");
      exit(1);
    }
  } else if (cmd->bg) {
    src = open("/dev/null", O_RDONLY);
    if (src == -1) {
      perror("open /dev/null for read");
      exit(1);
    }
  }

  /* Opens the output file as write only, and creates the file if it does not exist.
   * If a background command has no specified output, opens /dev/null. */
  if (cmd->output_file != NULL) {
    dest = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest == -1) {
      perror("open output file for write");
      exit(1);
    }
  } else if (cmd->bg) {
    dest = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest == -1) {
      perror("open /dev/null for write");
      exit(1);
    }
  }

  /* Performs redirection to the src and dest streams. */
  if (cmd->input_file != NULL || cmd->bg) {
    if (dup2(src, 0) == -1) {
      perror("dup2 input");
      exit(1);
    }
  }
  if (cmd->output_file != NULL || cmd->bg) {
    if (dup2(dest, 1) == -1) {
      perror("dup2 output");
      exit(1);
    }
  }

  return;
}

void kill_children(DA *da){
  /* Kills all processes associated with the process IDs stored in the array using the SIGKILL signal.
   *
   * Parameters:
   *   *da: dynamic array filled with process IDs */

  for (int i = 0; i < da->num_elem; ++i) {
    if(kill(da->array[i], SIGKILL) != 0) perror("kill");
  }

  return;
}

void check_children(DA *da){
  /* Checks the status of all child processes associated with the process IDs stored in the array. If the process
   * has terminated, prints the exit status or signal from the process and removes it from the array.
   *
   * Parameters:
   *   *da: array of child process IDs */

  int pid_status;
  int child_status;
  int exit_status;
  int exit_signal;
  
  for (int i = 0; i < da->num_elem; ++i) {

      /* Checks if the child has completed, but does not wait for completion using the WNOHANG flag.
       * If the child process has completed, it will be reaped now. */
      pid_status = waitpid(da->array[i], &child_status, WNOHANG);

      if (pid_status == -1) perror("waitpid on child process");

      else if (pid_status != 0) {
        if (WIFEXITED(child_status)) {
          exit_status = WEXITSTATUS(child_status);
          printf("background pid %d is done: exit value %d\n", pid_status, exit_status);
          fflush(NULL);
        } else {
          exit_signal = WTERMSIG(child_status);
          printf("background pid %d terminated by signal %d\n", pid_status, exit_signal);
          fflush(NULL);
        }

        da_remove(da, i);
        --i;
      }
    }


  return;
}

void toggle_fg_mode() {
  /* Signal handler function for SIGTSTP. This toggles a global boolean variable on or off whenever SIGSTSP is 
   * delivered. */

  fg_mode = !fg_mode;

  if (fg_mode) {
    printf("\nEntering foreground-only mode (& is now ignored)\n: ");
  } else {
    printf("\nExiting foreground-only mode\n: ");
  }
  fflush(NULL);

  return;
}
