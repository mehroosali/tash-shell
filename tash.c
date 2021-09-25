#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

/************
OUTLINE

(FUNCTION)
processCommand: returns number of children created
    Process commandLine by ' \t'
        InBuildCommands()
        else: fork
            Process commandLine further by '>'
                Do appropriate redirection
            Child:
                Execute
                SafetyExit (children all exit when error rather than returning)
            Parent:
                return 1
return 0

(FUNCTION)
processCommandLine: returns void
    Process commandLine by '&'(parallel), creating an array
    While commands remain:
        processCommand and count children created
    Parent: wait for all children
return

(MAIN)
Batch or not?
batch?: while{commandLine}:processCommandLine    //Error only if we get >1 arg, or a bad batch file
else promt, while{commandLine}: processCommandLine, Prompt
Program end

*************/

/*** GLOBAL VARIABLES ***/
char error_message[30] = "An error has occurred\n";
char* path[256] = {"/bin"}; //allows 256 individual path variables

//returns number of children started
int processCommand(char* command)   {
    
    //if null command just return
    if(command == NULL)   {
        return(0);
    }

    /***** SEPERATE BY WHITESPACE *****/
    //first count how many words there are
    int wordCount = 0;
    int newWord = 1;
    int w;
    for(w = 0; w < strlen(command); w++) {
        //if we hit white spaces, the next non-whitespace is a new word
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
    w = 0;
    myargs[w] = strtok(command, " \t");
    while(myargs[w]!=NULL){
        w++;
        myargs[w] = strtok(NULL, " \t");
    }

    //if null first argument just return
    if(myargs[0] == NULL)   {
        return(0);
    }

    /***** COMMAND TYPE *****/
    /*** EXIT ***/
    if(strcmp(myargs[0],"exit") == 0){
        if(wordCount > 1){
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
            exit(0);
        }
        return 0;
    }
    /*** CD ***/
    else if(strcmp(myargs[0], "cd") == 0){
        if(wordCount == 1 || wordCount > 2 || chdir(myargs[1]) != 0 ){
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        return 0;
    }
    /*** PATH ***/
    else if(strcmp(myargs[0], "path") == 0) {
        int i = 0;
        int n = sizeof(myargs)/sizeof(myargs[0]);
        /**check if user enters just path. if so then set path array empty 
        so that all commands fail else add each path to the path variable **/
        if(myargs[1] != NULL) {
            for(i=0;i<n-2;i++) {
                path[i] = strdup(myargs[i+1]);
            }
        }
        path[i] = NULL;
        return 0;
    }
    /*** EXTERNAL ***/
    else {
        int rc = fork();

        if(rc==0){  //child executes command

            /***** REDIRECTION *****/
            //Check for redirections (which don't need spaces, and take only one argument, which is a path)
            int i;
            for(i = 0; i<wordCount; i++) {
                char* ret = strchr(myargs[i], '>');
                if (ret!=NULL)  {
                    char* output;
                    //make sure there is an argument before the redirection
                    if(i==0 && strcmp(myargs[i],ret)==0)    {
                        //error and exit child
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(-1);
                    }

                    if(i==wordCount-1)  {   //i is the last word
                        //copy output
                        output = strdup(ret+1);
                        //either remove the argument or terminate it early
                        if(strcmp(myargs[i],ret)==0)    {
                            myargs[i] = NULL;
                        } else  {
                            myargs[i][strcspn(myargs[i], ">" )] = '\0'; 
                        }
                    } else if (i==wordCount-2)   {   //i is in second to last word
                        if (strcmp(ret, ">") != 0)  {   //was not last character
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(-1); //child exits
                        }
                        output = strdup(myargs[i+1]);
                        myargs[i+1] = NULL;
                        //either remove the argument or terminate it early
                        if(strcmp(myargs[i],ret)==0)    {
                            myargs[i] = NULL;
                        } else  {
                            myargs[i][strcspn(myargs[i], ">" )] = '\0'; 
                        }
                    } else  {   //had more than one word left
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(-1); //child exits
                    }

                    //change stdout
                    close(1);
                    int fd = open(output, O_WRONLY|O_CREAT|O_APPEND, 0644);
                    if (fd == -1)    {
                        //file redirection failed
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(-1); //child exits
                    }
                    //change stderr
                    close(2);
                    open(output, O_WRONLY|O_CREAT|O_APPEND, 0644);
                    break;
                }
            }
        
            //ignore empty commands
            if(myargs[0] == NULL || strcmp(myargs[0],"\0")==0 || strcmp(myargs[0],"")==0)   {
                exit(0);
            }

            //path search logic
            char* binaryPath;
            size_t j = 0;
            int n = sizeof(path) / sizeof(path[0]);
            
            for (j = 0; j < n;  j++) {
                if(path[j] == NULL || strcmp(path[j],"")==0) {
                    break;
                }
                binaryPath = strcat(strcat(strdup(path[j]),"/"), myargs[0]);
                if(access(binaryPath, X_OK) == 0) {
                    execv(binaryPath, myargs);
                }
            }
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(-1); //child exits
        }
        return 1;
    }
    return 0;
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

    //check that there isn't an empty command except possibly last
    for(i = 0; i<commandCount-1; i++)   {
        if(commands[i] == NULL || commands[i][0]=='\0')   {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
        char *checker = strdup(commands[i]);
        if(strtok(checker, " \t")==NULL)    {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
    }

    int children = 0;

    /*** EXECUTE/PROCESS EACH COMMAND ***/
    for (i = 0; i<commandCount; i++) {
        children += processCommand(commands[i]);
    }

    /*** WAIT FOR ALL CHILDREN ***/
    for (i = 0; i<children; i++)    {
        //If a children already exited, -1 will be gotten from wait but still works.
        wait(NULL);
    }

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
