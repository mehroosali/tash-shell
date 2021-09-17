#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/************
ROUGH OUTLINE

(FUNCTION)
OneCommandLine:
    Process commandLine by '&'(parallel), creating an array
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

/*** GLOBAL VARIABLES ***/
char error_message[30] = "An error has occurred\n";
//TODO make path variable better
char* path[];


void processCommand(char* command)   {
    /***** SEPERATE BY WHITESPACE *****/
    //first count how many words there are
    int wordCount = 1;
    for(int w = 0; w < strlen(command); w++) {
        //TODO this must take any number of spaces...  
        if(command[w] == ' ')  {
            wordCount++;  
        }
    }
    //create an array with wordCount many words (and one more for null terminated)
    char* myargs[wordCount+1];
    //tokenize line into myargs[] until null (and save the null too)
    //TODO this should simply split base on tabs and multiple spaces
    //TODO verify it does whitespaces as desired
    int w = 0;
    myargs[w] = strtok(command, " \t");
    while(myargs[w]!=NULL){
        w++;
        myargs[w] = strtok(NULL, " \t");
    }


    /***** COMMAND TYPE *****/
    /*** EXIT ***/
    if(strcmp(myargs[0],"exit") == 0){
        if(wordCount > 1){
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
            exit(0);
        }
    }
    /*** CD ***/
    else if(strcmp(myargs[0], "cd") == 0){
        if(wordCount == 1 || wordCount > 2 || chdir(myargs[1]) != 0 ){
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    /*** PATH ***/
    else if(strcmp(myargs[0], "path") == 0){
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
    /*** EXTERNAL ***/
    else {
        int rc = fork();

        if(rc==0){  //child executes command

            /***** REDIRECTION *****/
            //Check for redirections (which don't need spaces, and take only one argument, which is a path)
            for(int i = 1; i<wordCount; i++)    {
                char* ret = strchr(myargs[i], '>');
                if (ret!=NULL)  {
                    if(i<wordCount-1 || i==wordCount-1 && (strcmp(ret, '>') != 0))  {
                        //either there are more than two arguments left,
                        //or there is an argument left, and something after '>'
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        return;
                    } else {
                        //TODO redirect then break
                        break;
                    }
                }
            }

            execvp(myargs[0], myargs);

            //TODO the execvp failed
            exit(1);
        }
    }
}

void processCommandLine(char* commandLine)   {
    //replace newline character with '\0' as it causes problems in execvp
    commandLine[strcspn(commandLine, "\n" )] = '\0';


    /***** PARALLEL *****/
    int commandCount = 1;
    for(int i = 0; i < strlen(commandLine); i++) {  
        if(commandLine[i] == '&')  {
            commandCount++;
        }
    }
    //create an array with that many slots
    char* commands[commandCount];


    /*** EXECUTE/PROCESS EACH COMMAND ***/
    for(int i = 0; i<commands; i++) {
        //TODO process and execute command i
        processCommand(commands[i]);
    }
    

    /***** WAIT FOR ALL CHILDREN *****/
    int status = 0;
    pid_t wpid;
    while ((wpid = wait(&status)) != -1);

    //finished processing a command line
    return;
}


int main(int argc, char* argv[]) {

char* line;
size_t len=0;
ssize_t line_len;

/*
checking program arguments to check if batch mode
If batch mode, we create a file pointer to the existing file
else let the file pointer point to stdin for interative mode.
*/
if(argc == 1){
    //interactive mode
    printf("tash> ");
    while((line_len = getline(&line,&len,stdin)) != -1)    {
        processCommandLine(line);
        printf("tash> ");
    }
} else if(argc == 2) {
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    } else { 
        while((line_len = getline(&line,&len,fp)) != -1)    {
            processCommandLine(line);
        }
    }
} else {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}

return 0;
}
