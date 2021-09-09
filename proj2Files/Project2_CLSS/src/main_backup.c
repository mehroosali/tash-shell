#include <proj2.h>

int main(int argc, char *argv[], char *env[])	{

	int code;

	//PATH/Env Setup
	char *path = malloc(256);
	char *home = malloc(128);
	char pDir[32][64];
	//iterate through the env to find the PATH
	for(int i = 0; env[i]!=NULL; i++)	{
		char* substr = malloc(5);
		strncpy(substr, env[i], 5);
		if(strcmp(substr,"PATH=")==0)	{
			strcpy(path, env[i]+5);
			printf("Env-%d	:	PATH=%s\n", i, path);
		} else if (strcmp(substr,"HOME=")==0)	{
			strcpy(home, env[i]+5);
			printf("Env-%d	:	HOME=%s\n", i, home);
		}
	}
	//tokenize the PATH into the various directories stored into *pDir[]
	char *d = strtok(path, ":");
	for(int i = 0; d!=NULL; i++)	{
		strcpy(pDir[i],d);
		printf("pDir-%d	:	%s\n",i, pDir[i]);
		d = strtok(NULL, ":");
	}
	//Finished PATH/Env Setup

	//Start looping and taking commands
	while(1)	{
		//initialize a command array of strings
		char *args[32];
		for(int i = 0; i<32; i++)	{
			args[i] = (char*)malloc(64*sizeof(char));
		}

		//Get the command from the user and store it and the arguments in *command[]
			//rest the fd
		printf("input a sh command line : ");
		char line[128];
		fgets(line, 128, stdin);	//Should get an input line...
		line[strlen(line)-1] = 0; //kills '\n' at the end
		d = strtok(line, " ");
		int len = 0;	//using a while loop like this allows me to make sure the list is null terminated
		while(d!=NULL)	{
			//just an argument
			strcpy(args[len],d);
			d = strtok(NULL, " ");
			len++;
		}
		args[len] = NULL;

		//call the command
		printf("Command: %s", args[0]);
		for(int i = 1; args[i]!=NULL; i++)	{
			printf(" arg%d:%s", i, args[i]);
		}
		printf("\n");

		//handle commands
		if(strcmp(args[0],"cd")==0)	{
			//handle cd
			if(strcmp(args[1],"\0")==0)	{
				code = chdir(home);
			} else{
				code = chdir(args[1]);
			}
		}
		else if (strcmp(args[0],"exit")==0)	{
			//handle exit
			exit(0);
		}
		else{
			//handle external commands
			int pid = fork();
			if(pid){	//parent
				pid = wait(&code);
			} else{		//child
				//determine IO changes
				for(int i = 0; args[i]!=NULL; i++)	{
					if(strcmp(args[i], "<")==0)	{
						//input
						if(args[i+1]==NULL)	{
							exit(520001);
						} else if(access(args[i+1], F_OK) != 0)	{
							exit(520002);
						}
						printf("input from %s\n", args[i+1]);
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
							exit(520001);
						}
						printf("output to %s\n", args[i+1]);
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
							exit(520001);
						}
						printf("append to %s\n", args[i+1]);
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
						execve(pathname, args, env);
					}
				}

				//file was not found
			}
		}
		//output based on exit code
		switch (code) {
			case 0:
				printf("-------Exit Code %d : successful-------\n", code);
				break;
			case 520001:
				printf("-------Exit Code %d : ioFile not specified-------\n", code);
				break;
			case 520002:
				printf("-------Exit Code %d : No such ioFile exists-------\n", code);
				break;
			default:
				printf("-------Exit Code %d : unlabeled error-------\n", code);
		}
	}
	//Finish looping and taking commands
}
