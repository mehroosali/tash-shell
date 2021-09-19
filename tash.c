#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
char* path[10] = {"/bin"};

void processCommand(char* command)   {
    /***** SEPERATE BY WHITESPACE *****/
    //first count how many words there are
    int wordCount = 0;
    int newWord = 1;
    int w;
    for(w = 0; w < strlen(command); w++) {
        //if we hit white spaces, the next non-whitespace is a new word
        //TEST this might count end of string characters?
        if((command[w]==' ')||(command[w]=='\t'))  {
            newWord = 1;
        } else if (newWord) {
            wordCount++;
            newWord = 0;
        }
    }
    //create an array with wordCount many words (and one more for null terminated)
    char* myargs[wordCount+1];
    //tokenize line into myargs[] until null (and save the null too)
    //TEST it does whitespaces and tabs as desired
    w = 0;
    myargs[w] = strtok(command, " \t");
    while(myargs[w]!=NULL){
        w++;
        myargs[w] = strtok(NULL, " \t");
    }

    //printf("Word Count - %d \n",wordCount);
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
    else if(strcmp(myargs[0], "path") == 0) {
        int i;
        int n = sizeof(myargs)/sizeof(myargs[0]);
        /**check if user enters just path. if so then set path array NULL 
        so that all commands fail else add each path to the path variable **/
        if(myargs[1] != NULL) {
            for(i=0;i<n-2;i++) {
                path[i] = myargs[i+1];
            }
        } else {
            path[0] = NULL;
        }
        
    }
    /*** EXTERNAL ***/
    else {
        int rc = fork();

        if(rc==0){  //child executes command

            /***** REDIRECTION *****/
            //Check for redirections (which don't need spaces, and take only one argument, which is a path)
            int i;
	    for(i = 1; i<wordCount; i++) {
                char* ret = strchr(myargs[i], '>');
                if (ret!=NULL)  {
                    char* output;
                    if(i==wordCount-1)  {   //i is the last word
                        //TEST that this points to what I want it to
                        //copy output
                        strcpy(output, ret+1);
                        //remove ret from myargs[i]
                        ret = '\0';
                    } else if (i=wordCount-2)   {   //i is in second to last word
                        if (strcmp(ret, ">") != 0)  {   //was not last character
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            return;
                        }
                        strcpy(output, myargs[i+1]);
                        myargs[i+1] = NULL;
                    } else  {   //had more than one word left
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        return;
                    }
                    //attempt to make it work

                    //change stdout
                    close(1);
                    int fd;
                    if (fd = open(output, O_WRONLY|O_CREAT|O_APPEND, 0644) == -1)    {
                        //file redirection failed
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        return;
                    }
                    //change stderr
                    close(2);
                    open(output, O_WRONLY|O_CREAT|O_APPEND, 0644);
                    break;
                }
            }
        
        //path search logic
            char* binaryPath;
            size_t j = 0;
            int n = sizeof(path) / sizeof(path[0]);
            printf("size");
            if(path[j] != NULL) {
                for (j = 0; j < n;  j++) {
                    binaryPath = strcat(strcat(strdup(path[j]),"/"), myargs[0]);
                    if(access(binaryPath, X_OK) == 0) {
                        break;
                    }
                }
            }
           execv(binaryPath, myargs)
           
           write(STDERR_FILENO, error_message, strlen(error_message));
           exit(1);
        }
    }
    return;
}

void processCommandLine(char* commandLine)   {
    //replace newline character with '\0' as it causes problems in execvp
    commandLine[strcspn(commandLine, "\n" )] = '\0';

    /***** PARALLEL *****/
    int commandCount = 1;
    int i;
    for(i = 0; i < strlen(commandLine); i++) {  
        if(commandLine[i] == '&')  {
            commandCount++;
        }
    }
    
    //create an array with that many slots
    char* commands[commandCount];
    //tokenize commanddLine into commands[]
    commands[0] = strtok(commandLine, "&");
    for(i = 1; i<commandCount; i++){
        commands[i] = strtok(NULL, "&");
    }

    //exprintf("Command Count - %d\n", commandCount);
    /*** EXECUTE/PROCESS EACH COMMAND ***/
    for (i = 0; i<commandCount; i++) {
        processCommand(commands[i]);
    }

    wait(NULL);

    /***** WAIT FOR ALL CHILDREN ****
    int status = 0;
    pid_t wpid;
    while ((wpid = wait(&status)) != -1);
    */
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
