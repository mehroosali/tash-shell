#include <stdio.h>
#include <stdlib.h>

int main(void) {
char* myargs[2];
char* line;
size_t len=0;
ssize_t line_len;

myargs[1]=NULL;

printf("tash> ");

while((line_len = getline(&line,&len,stdin)) != -1){

myargs[0]=strtok(line,"\n");

int rc = fork();

if(rc==0){

execvp(myargs[0], myargs);

} else { 
int wc = wait(NULL);
printf("tash> ");
}
}
return 0;
}
