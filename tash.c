#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(void) {
char* line;
size_t len=0;
ssize_t line_len;

printf("tash> ");

while((line_len = getline(&line,&len,stdin)) != -1){
//chop off newline character as it causes problems in execvp
line[strcspn(line, "\n" )] = '\0';  //look for the first instance of '\n' and replace it with terminating character.

//first count how many words there are
int wordCount = 1;
for(int i = 0; i < strlen(line); i++) {  
    if(line[i] == ' ')  {
        wordCount++;  
    }
}

//create an array with wordCount many words (and one more for null terminated)
char* myargs[wordCount+1];
printf("There are %d word slots\n", wordCount+1);

//tokenize line into myargs[] until null (and save the null too)
int i = 0;
myargs[i] = strtok(line, " ");
while(myargs[i]!=NULL)	{
    i++;
    myargs[i] = strtok(NULL, " ");
}

//fork a process
int rc = fork();

//child executes command
if(rc==0){

execvp(myargs[0], myargs);

} else { //parent waits until child, then preps for more input
int wc = wait(NULL);
printf("tash> ");
}
}   //while loop

return 0;
}
