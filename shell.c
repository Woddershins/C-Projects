#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
 

//A simple shell, but quite breakable
//some starter code was provided by a wonderful professor of mine, essentially the while(should_run) loop


#define MAX_LINE 80 /* 80 chars per line, per command */
#define RCV_END 0		/* receiving end of pipe_fd */
#define WRT_END 1		/* writing end of pipe_fd */
 
int main(void){
	
	FILE *file;
	char *args[MAX_LINE/2 + 1] = {NULL};
	
	char command[MAX_LINE];
	char history[MAX_LINE]="";
	char wrt_msg[MAX_LINE/2];

	int io_fd;
	int out_fd;
	int in_fd;
	int pipe_fd[2];

	int should_run = 1;
	int pipe_exists = 0;
	pid_t pid;

	//main shell loop, runs until should_run is set to 0 with exit command
  while (should_run){
		int arg_count=0;
  	printf("osh>");
    fflush(stdout);

		//get user input string
		fgets(command, MAX_LINE, stdin);

		//checks for empty command
		if(strcmp("", command)==0){
			continue;
		}


		/*check for history command
			check if history is empty (declared empty)
			indicate if there is previous command or set command to history
			*/
		if(strncmp("!!", command, 2)==0){
			if(strncmp("", history, 1)==0){
				printf("No Previous Command\n");
				continue;
			}else{
				strcpy(command,history);
			}
		}

		//checks for exit
		if(strncmp("exit", command, 4)==0){
				should_run=0;
				continue;
		}

		//sets history
		strcpy(history, command);

		//saves stdin and stdout before fork
		out_fd = dup(STDOUT_FILENO);
		in_fd = dup(STDIN_FILENO);

		//fork process
		pid = fork();
		//in case of error forking
		if(pid<0){
			fprintf(stderr, "Error forking!");
		}

		//primary child process
		else if(pid==0){

		//create pipe
		if(pipe(pipe_fd)==-1){
			fprintf(stderr,"Error piping!");
		}

		//creates tokens from command
		//I wanted to create this as a function but had difficulty
			char *token = strtok(command, " \n");
			while(token != NULL) {
				args[arg_count]=token;
				token = strtok(NULL, " \n");
				arg_count++;
			}

			/*check for redirection ( > or < ) and pipe
				(redirect)
				saves STDIN/STDOUT file descriptors for later
				tries to open file with name retrieved at the next argument position
				gets file descriptor
				sets stdout to that file descriptor
				also sets the current argument to null, essentially truncating the args array
				(< command also sets arg[0] to file contents so it reads in)
				*/
			for(int i=0; i<arg_count; i++){
				if(strncmp(">", args[i], 1)==0){
					file = fopen(args[i+1], "w");
					io_fd = fileno(file);
					dup2(io_fd, STDOUT_FILENO);
					args[i]=NULL;
				}
				else if(strncmp("<", args[i], 1)==0){
					file = fopen(args[i+1], "r");
					io_fd = fileno(file);
					dup2(io_fd, STDIN_FILENO);
					args[i]=NULL;
					fgets(args[0],MAX_LINE/2,stdin);
				}
				//checks for pipe character, gets next argument for pipe_command
				else if(strncmp("|", args[i], 1)==0){
					pipe_exists = 1;
					strcpy(wrt_msg,args[i+1]);
					args[i]=NULL;
				}
			}

			//here i think i'd want to redirect stdout to pipe using dup2
			//this would let me get first command output and pipe it to stdin for second command
			//2 things need to be sent, the args for the pipe_command, and the output from the first command

			//execute command in argument tokens
			if(execvp(args[0], args) ==-1){
				fprintf(stderr, "Command %s not found\n", args[0]);
			}

			//child of child is only created when a pipe is detected
			if(pipe_exists){

				//checks if this is child process
				if(pid==0){
					pid = fork();
				}

				//checks for error
				if(pid<0){
					fprintf(stderr, "Error forking!");
				}

				//secondary parent process AKA primary child process
				if(pid>0){
					//write message from parent to child
					write(pipe_fd[WRT_END],wrt_msg,MAX_LINE/2+1);
					close(pipe_fd[WRT_END]);
					wait(NULL);
				}

				//secondary child process
				if(pid==0){
					//declare variables similar to original process
					int pipe_arg_count=0;
					char pipe_command[MAX_LINE];
					char *pipe_args[MAX_LINE/2 + 1] = {NULL};

					//read in user input after the | character
					read(pipe_fd[RCV_END], pipe_command ,MAX_LINE/2);

					//create tokens from pipe_command
					char *token = strtok(pipe_command, " \n");
					while(token != NULL) {
						pipe_args[pipe_arg_count]=token;
						token = strtok(NULL, " \n");
						pipe_arg_count++;
					}
					
					//close receiving end of pipe
					close(pipe_fd[RCV_END]);

					//here i need to redirect pipe output to stdin to run the pipe_command
					
					//execute command in argument tokens
					if(execvp(pipe_args[0], pipe_args) ==-1){
						fprintf(stderr, "Command %s not found\n", args[0]);
					}
				}
			}
		}

		//primary parent process
		else if(pid>0){
			wait(NULL);
		}

		//file clean up
		dup2(out_fd,STDOUT_FILENO);
		dup2(in_fd,STDIN_FILENO);
		close(out_fd);
		close(in_fd);
		fflush(stdout);
		fflush(stdin);
		arg_count=0;
	}
	return 0;
}