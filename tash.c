#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*** GLOBAL VARIABLES ***/
char error_message[30] = "An error has occurred \n";


int processCommand()   {


    return 0;
}

int tester(int argc, char* argv[]) {
    printf ("Hello World");
    char *line = "Wow so  many \t  words\tnow";
    int i = 0;
    char *myargs[15];
    myargs[i] = strtok(line, " \t");
    while (myargs[i] != NULL) {
        i++;
        myargs[i] = strtok (NULL, " ");
        printf ("%s ", myargs[i]);
    }
    return 0;
}

int main(int argc, char* argv[]) {

FILE * fp;
char* line;
size_t len=0;
ssize_t line_len;
int isbatch = 0; //creating a flag to check if the mode is a batch mode

/*
checking program arguments to check if batch mode
If batch mode, we create a file pointer to the existing file
else let the file pointer point to stdin for interative mode.
*/
if(argc == 1){
    fp = stdin;
} else if(argc == 2) {
    fp = fopen(argv[1], "r");
    if(fp == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }
    isbatch = 1;
} else {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(0);
}

// printing tash> if interactive mode 
if (isbatch == 0){
    printf("tash> "); 
}

/************
ROUGH OUTLINE

(FUNCTION)
OneCommandLine:
    Process commandLine by '&' parallel
    While commands remain:
        Process commandLine by ' \t'
        InBuildCommands()
        else: fork
            Process commandLine further by '>'
                Do appropriate redirection
            Child:
                Check for redirect
                Execute
                SafetyExit
        Make sure previous command is eaten
    All commands forked
    Parent: wait for all children
Return

(MAIN)
Batch or not?
batch?: while{commandLine}:OneCommandLine    //Error only if we get >1 arg, or a bad batch file
else promt, while{commandLine}: OneCommandLine, Prompt
Program end


EOF exits us
*************/

//fp is passed here. 
while((line_len = getline(&line,&len,fp)) != -1){

    int ret = processCommand();

//chop off newline character as it causes problems in execvp
line[strcspn(line, "\n" )] = '\0';  //look for the first instance of '\n' and replace it with terminating character.

//first count how many words there are
int wordCount = 1;
int i;
for(i = 0; i < strlen(line); i++) {  
    if(line[i] == ' ')  {
        wordCount++;  
    }
}

//create an array with wordCount many words (and one more for null terminated)
char* myargs[wordCount+1];

//tokenize line into myargs[] until null (and save the null too)
//this should simply split base on tabs and multiple spaces
//verify it does whitespaces as desired
i = 0;
myargs[i] = strtok(line, " \t");
while(myargs[i]!=NULL){
    i++;
    myargs[i] = strtok(NULL, " ");
}

//TODO all of these (except exit) need work to be looping nicely with our external commands.
//TODO probably just a refactor actually.

//checking for inbuilt exit command
if(strcmp(myargs[0],"exit") == 0){
    if(wordCount > 1){
        write(STDERR_FILENO, error_message, strlen(error_message));
    } else {
        exit(0);
    }
}

//checking for inbuilt cd command
if(strcmp(myargs[0], "cd") == 0){
    if(wordCount == 1 || wordCount > 2 || chdir(myargs[1]) != 0 ){
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

//checking for inbuild path command
if(strcmp(myargs[0], "path") == 0){
    char* args = malloc(255*wordCount); //malloc ~255 characters per path
    strcat(args, "PATH=");

    //append arguments
    printf("%d words\n", wordCount);
    for(int i = 1; i<wordCount; i++)  {
        strcat(args, " ");
        strcat(args, myargs[i]);
    }
    //TODO this might need to have no space after the equals
    printf("%s\n", args);
    putenv(args);
    free(args);
}

//fork a process
int rc = fork();

if(rc==0){  //child executes command
    //Check for redirections (which don't need spaces, and take only one argument, which is a path)
    for(int i = 1; i<wordCount; i++)    {
        char* ret = strchr(myargs[i], '>');
        if (ret!=NULL)  {
            if(i<wordCount-1 || i==wordCount-1 && (strcmp(ret, '>') != 0))  {
                //either there are more than two arguments left, or there is an argument left, and something after '>'
                printf("errored \n");
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else {
                //TODO redirect then break
                break;
            }
        }
    }
    execvp(myargs[0], myargs);
    //TODO the execvp failed
    exit(1);
} else {    //parent waits until child, then preps for more input
    int wc = wait(NULL);
    // printing tash> if interactive mode 
    if (isbatch == 0){
        printf("tash> "); 
    }
}
}   //while loop ends

// close the opened file if batch mode 
if (isbatch == 1){
    fclose(fp);
}

return 0;
}
