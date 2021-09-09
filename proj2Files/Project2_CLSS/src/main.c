#include <proj2.h>

//this method actually runs the commands
int processCommand(char *commandLine, char *env[])	{
		//idea to fix this stuff, didn't work on it's own
	char* argLine = malloc(128);
	strcpy(argLine, commandLine);

	//PATH/PWD Setup
	char *path = malloc(256);
	char pDir[32][64];
	char *wd = malloc(128);
	//iterate through the env to find the PATH
	for(int i = 0; env[i]!=NULL; i++)	{
		char* substr = malloc(5);
		strncpy(substr, env[i], 5);
		if(strcmp(substr,"PATH=")==0)	{
			strcpy(path, env[i]+5);
		} else if (strncmp(substr,"PWD=", 4)==0)	{
			strcpy(wd, env[i]+4);
		}
	}
	//tokenize the PATH and PWD into the various directories stored into *pDir[]
	strcpy(pDir[0],wd);
	char *d = strtok(path, ":");
	int pNum = 0;
	while(d!=NULL)	{
		strcpy(pDir[pNum],d);
		d = strtok(NULL, ":");
		pNum++;
	}
	//add the working directory to the list
	strcpy(pDir[pNum],wd);
	strcpy(pDir[pNum+1],"\0");
	//Finished PATH/PWD Setup


	//initialize a command array of strings
	char *args[32];
	for(int i = 0; i<32; i++)	{
		args[i] = (char*)malloc(64*sizeof(char));
	}

	//parsing the argument line into the command and the various arguments
	d = strtok(argLine, " ");
	int len = 0;	//using a while loop like this allows me to make sure the list is null terminated
	while(d!=NULL)	{
		//just an argument
		strcpy(args[len],d);
		d = strtok(NULL, " ");
		len++;
	}
	args[len] = NULL;

	//handle commands
	//determine IO changes
	for(int i = 0; args[i]!=NULL; i++)	{
		if(strcmp(args[i], "<")==0)	{
			//input
			if(args[i+1]==NULL)	{
				exit(51);
			} else if(access(args[i+1], F_OK) != 0)	{
				exit(52);
			}
				//printf("input from %s\n", args[i+1]);
			close(0);
			int fd = open(args[i+1], O_RDONLY);
			//clear the two entries from the arg list
			while(args[i+2]!=NULL)	{
				args[i]=args[i+2];
				i++;
			}
			args[i]=NULL;

		}
		else if(strcmp(args[i], ">")==0)	{
			//output
			if(args[i+1]==NULL)	{
				exit(51);
			}
				//printf("output to %s\n", args[i+1]);
			close(1);
			int fd = open(args[i+1], O_WRONLY|O_CREAT, 0644);
			//clear the two entries from the arg list
			while(args[i+2]!=NULL)	{
				args[i]=args[i+2];
				i++;
			}
			args[i]=NULL;
		}
		else if(strcmp(args[i], ">>")==0)	{
			//append
			if(args[i+1]==NULL)	{
				exit(51);
			}
				//printf("append to %s\n", args[i+1]);
			close(1);
			int fd = open(args[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
			//clear the two entries from the arg list
			while(args[i+2]!=NULL)	{
				args[i]=args[i+2];
				i++;
			}
			args[i]=NULL;

		}
	}
	
	//Hunt for the file...
	for(int i = 0; strcmp(pDir[i],"\0"); i++)	{
		char *pathname = malloc(128);
		strcat(pathname, pDir[i]);	//append pathname with the current path directory to check
		strcat(pathname, "/"); //append the pathname with '/'
		strcat(pathname, args[0]); //append the pathname with the filename
		if( access(pathname, F_OK) == 0 ) {
			//found the file
			//check if it is a sh or ELF file
			FILE *fp = fopen(pathname, "r");
			char buff[256];
			fgets(buff, 256, (FILE*)fp);
			fclose(fp);
			if(strncmp(buff, "#!", 2))	{
				//not a sh file
				execve(pathname, args, env);
			}
			else	{
				//is a sh file
				char *bashArgs[] = {"bash", pathname};
				execve("/usr/bin/bash", bashArgs, env);
			}
		}
	}
	//no such command found
	exit(53);
}

//this method can be recursive, and pipes the commands
int commandPiper(char *commandLine, char *env[])	{
	int returnCode;

	//checking if there is piping to be done
	if(strchr(commandLine, '|')!=NULL)	{
		//set-up head and tail. Tail may be recursive.
		char *tail = commandLine;
		char *head = strtok_r(tail, "|", &tail);
		//set-up the pipe
		int pd[2];
		pipe(pd);	//creates a pipe
		int pid = fork(); //forked a child
		if(pid==0)	{
			//child becomes the pipe writer
			close(pd[0]);	//can't read now
			close(1);
			dup(pd[1]);		//write to pipe now
			close(pd[1]);	//pipe fd has been copied
			returnCode = processCommand(head, env);
		}
		else	{
			//parent becomes the pipe reader
			close(pd[1]);	//can't write now
			close(0);
			dup(pd[0]);		//read from pipe now
			close(pd[0]);	//pipe fd has been copied
			returnCode = commandPiper(tail, env);	//recursive call that will make sure there isn't another pipe
		}
	}
	else	{
		//no more piping to be done, run as usual
		returnCode = processCommand(commandLine, env);
	}
	printf("Exit commandPiper: %d\n", returnCode);
	return returnCode;
}

int main(int argc, char *argv[], char *env[])	{

	int code;	//output code
	int printCode = 0;	//toggle for output codes on/off

	//establish HOME
	char *home = malloc(128);
	//iterate through the env to find HOME
	for(int i = 0; env[i]!=NULL; i++)	{
		char* substr = malloc(5);
		strncpy(substr, env[i], 5);
		if (strcmp(substr,"HOME=")==0)	{
			strcpy(home, env[i]+5);
		}
	}

	//Start looping and taking commands
	while(1)	{

		//Get the command from the user and chop it into the appropriate command lines
		printf("$ ");
		char line[128];
		fgets(line, 128, stdin);	//Should get an input line...
		line[strlen(line)-1] = 0; //kills '\n' at the end

		//parse into command lines
		char *remainder = line;
		char *commandLine;
		while(commandLine = strtok_r(remainder, ";", &remainder))	{
			//execute the next command line of the whole line

			//if the command is exit, we need to do that here
			char *tmp = malloc(32);
			strcpy(tmp, commandLine);
			char *command = strtok(tmp, " ");
			if (strcmp(command,"exit")==0)	{
				//handle exit
				exit(-50);
			}
			else if(strcmp(command,"cd")==0)	{
				//handle cd
				command = strtok(NULL, " ");
				if(command == NULL)	{
					code = chdir(home);
				} else{
					code = chdir(command);
				}
			}
			else if(strcmp(command,"printc")==0)	{
				//handle printCodes toggle
				if(printCode == 0)	{
					printCode = 1;
				} else{
					printCode = 0;
				}
			}
			else	{
				//fork to go into the other commands
				int pid = fork();
				if(pid){	//parent
					pid = wait(&code);
					code = WEXITSTATUS(code);
				} else	{
					code = commandPiper(commandLine, env);
				}
			}

			//output based on exit code
			if(printCode)	{
				switch (code) {
					case 0:
						printf("-------Exit Code %d : successful-------\n", code);
						break;
					case 51:
						printf("-------Exit Code %d : ioFile not specified-------\n", code);
						break;
					case 52:
						printf("-------Exit Code %d : No such ioFile exists-------\n", code);
						break;
					case 53:
						printf("-------Exit Code %d : No such command found-------\n", code);
						break;
					default:
						printf("-------Exit Code %d : unlabeled error-------\n", code);
				}
			}
			
		}
		

		
	}
	//Finish looping and taking commands
}
