#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*** GLOBAL VARIABLES ***/
char error_message[30] = "An error has occurred\n";
//TODO path variable array here


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


int main(int argc, char* argv[]) {

char* line;
ssize_t line_len;

/*
checking program arguments to check if batch mode
If batch mode, we create a file pointer to the existing file
else let the file pointer point to stdin for interative mode.
*/
if(argc == 1){
    //interactive mode
    printf("tash> ");
    while((line_len = getline(&line,&len,fp)) != -1)    {

        printf("tash> ");
    }
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

return 0;
}
