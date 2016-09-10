/* CS 370
   Project #2
   Gerardo Gomez-Martinez
   September 16, 2014

   Program acts as a Linux terminal shell. It prints a prompt containing the current working
   directory and waits for user input. It accepts commands from the user and executes until 
   the user exits the shell. Also accepts the chnage directory command and chnages the 
   directory accordingly. It also keeps a history of the last 10 commands executed and lets
   the user cycle through them using the arrow keys. 
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>

void printPrompt();

int main()
{
  int pid; /* Process ID of parent and child processes */
  int pipeid; /* Process ID used for pipe communication */
  int status; /* Status flag of child process */
  int i, j, k; /* Loop control variables */
  int h = 0; /* Position to store last command entered on history array */
  int histAr; /* Position of current cycle on history array */
  int arPress = 0; /* Number of arrow presses */
  int er; /* If an error was encountered in current call */
  int items = 0; /* Number of items in history array */
  int bytesRead; /* Number of bytes read on read() call*/
  int fd[2]; /* File descriptors for pipe communication */
  char *buffer = (char *)calloc(256, sizeof(char)); /* Input buffer */
  char *inpt = (char *)calloc(256, sizeof(char)); /* Current command being executed */
  char *command[20]; /* Tokenized command being executed */
  char *pipecmd[20]; /* Tokenized second command to use in pipe */
  char *history[10]; /* Array to hold commands history */
  char *token; /* Tokens of current command */
  char *erMsg; /* Error message for perror() call */
  char *des = (char *)calloc(2, sizeof(char));
  bool ext = false; /* If the user has exited the shell */
  bool brk; /* If the user has entered the command */
  bool piping; /* If the pipe operator is in the command */
  struct termios origConfig; /* Original termios configuration */
  struct termios newConfig; /* Modified termios configuration */

  setbuf(stdout, NULL);
  
  /* Get original termios configuration */
  tcgetattr(0, &origConfig);
  newConfig = origConfig;

  /* Adjust the new configuration */
  newConfig.c_lflag &= ~(ICANON|ECHO);
  newConfig.c_cc[VMIN] = 5;
  newConfig.c_cc[VTIME] = 1;
  
  /* Set the new configuration */
  tcsetattr(0, TCSANOW, &newConfig);

  /* Allocate space for the history array */
  for (j = 0; j<10; j++)
    {
      history[j] = (char *)calloc(256, sizeof(char));
    }

  while (!ext)
    {
      printPrompt();
      brk = false;
      piping = false;
      j = 0;
      k = 0;

      while (!brk)
	{
	  /* Reads input from the user and process it */
	  bytesRead = read(0, buffer, 10);
	  if (bytesRead == 1)
	    {
	      /* If user presses enter break */
	      if (buffer[0] == '\n')
		{
		  printf("%c", buffer[0]);
		  inpt[j] = '\n';
		  brk = true;
		}
	      /* User enters backspace or delete */
	      else if (buffer[0] == 127 || buffer[0] == 8)
		{
		  if (j > 0)
		    {
		      printf("%c%c%c", '\b', ' ', '\b');
		      j--;
		      inpt[j] = '\b';
		    }
		}
	      else
		{
		  printf("%c", buffer[0]);
		  inpt[j] = buffer[0];
		  j++;
		}
	    }
	  else if (bytesRead == 2)
	    {
	      if (buffer[0] != '\n' && buffer[1] != '\n')
		{
		  printf("%c%c", buffer[0], buffer[1]);
		  inpt[j] = buffer[0];
		  j++;
		  inpt[j] = buffer[1];
		  j++;
		}
	      else if (buffer[0] == '\n')
		{
		  brk = true;
		}
	      else
		{
		  printf("%c%c", buffer[0], buffer[1]);
		  inpt[j] = buffer[0];
		  j++;
		  brk = true;
		}
	    }
	  else
	    {
	      /* Arrow pressed */
	      if (bytesRead == 3)
		{
		  if(buffer[0] == 27 && buffer[1] == 91 && buffer[2] == 65)
		    {
		      /* Up arrow */
		      if (arPress < items)
			{
			  arPress++;
			  for(k = 0; k < j; k++)
			    {
			      printf("%c%c%c", '\b', ' ', '\b');
			    }
			  histAr--;
			  if (histAr < 0)
			    histAr = 9;
			  printf("%s", history[histAr]);
			  strcpy(inpt, history[histAr]);
			  j = strlen(history[histAr]);
			  
			}
		    }
		  if(buffer[0] == 27 && buffer[1] == 91 && buffer[2] == 66)
		    {
		      /* Down arrow */
		      
		      if (arPress > 0)
			{
			  arPress--;
			  for(k = 0; k < j; k++)
			    {
			      printf("%c%c%c", '\b', ' ', '\b');
			    }
			  histAr++;
			  if (histAr > 9)
			    histAr = 0;
			  if (arPress == 0)
			    {
			      inpt[0] = '\n';
			      j = 0;
			    }
			  else
			    {
			      printf("%s", history[histAr]);
			      strcpy(inpt, history[histAr]);
			      j = strlen(history[histAr]);
			    }
			}
		    }
		}
	    }
	}

      /* If command has been input executes it */
      if(inpt[0] != '\n')
	{
	  strtok(inpt, "\n");
	  
	  /* Adds command to history array */
	  strcpy(history[h%10], inpt);
	  histAr = (h%10)+1;
	  arPress = 0;
	  h++;
	  items++;
	  if (items > 10)
	    items = 10;

	  token = strtok(inpt, " \n");
	  i = 0;

	  while (token != NULL && i < 19)
	    {
	      if (token[0] == '|')
		{
		  piping = true;
		  k = 0;
		}
	      else if (piping)
		{
		  pipecmd[k] = token;
		  k++;
		}
	      else
		{
		  command[i] = token;
		  i++;
		}
	      token = strtok(NULL, " \n");
	      
	    }

	  command[i] = NULL;
	  pipecmd[k] = NULL;

	  /* If change directory command chnage the current working directory */
	  if (strcmp(command[0], "cd") == 0)
	    {
	      er = chdir(command[1]);
	      if (er < 0)
		{
		  perror("cd");
		}
	    }
	  /* If command is exit asks the user if he's sure and executes accordingly */
	  else if (strcmp(command[0], "exit") == 0)
	    {
	      printf("Are you sure you want to exit (Y/N)? ");
	      bytesRead = read(0, des, 1);
	      printf("%c\n", des[0]);
	      if (des[0] == 'Y' || des[0] == 'y')
		ext = true;
	      else 
		{
		  while (!(des[0] == 'N'|| des[0] == 'n' || des[0] == 'Y' || des[0] == 'y'))
		    {
		      printf("Invalid command, please try again.\n");
		      printf("Are you sure you want to exit (Y/N)? ");
		      bytesRead = read(0, des, 1);
		      printf("%c\n", des[0]);
		      if (des[0] == 'Y' || des[0] == 'y')
			ext = true;
		    }
		}
	    }
	  else
	    {
	      pid = fork();

	      if (pid == 0)
		{
		  if (piping)
		    {
		      pipe(fd);
		      pipeid = fork();
		      
		      if (pipeid == 0)
			{
			  /* Child pipe process */
			  close(1);
			  
			  dup(fd[1]);

			  close(fd[0]);
			  execvp(command[0], command);

			  exit(0);

			}
		      else
			{
			  /* Parent pipe process */
			  close(0);

			  dup(fd[0]);

			  close(fd[1]);
			  er = execvp(pipecmd[0], pipecmd);
			  if (er < 0)
			    {
			      perror(pipecmd[0]);
			    }
			}

		      exit(0);
		    }
		  else
		    {
		      er = execvp(command[0], command);
		      if (er < 0)
			{
			  perror(command[0]);
			}
		      exit(0);
		    }
		}
	      else
		{
		  waitpid(pid, &status, WUNTRACED);
		}
	    }
	}
    }

  /* Restore the original termios configuration */
  tcsetattr(0, TCSANOW, &origConfig);
  free(buffer);

  return(0);
}

/* Prints the prompt containing the current working directory */
void printPrompt()
{
  char buffer[100];

  getcwd(buffer, sizeof(buffer));

  printf("%s>", buffer);
}
