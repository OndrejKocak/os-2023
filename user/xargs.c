#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc, char *argv[]){
    if(argc <= 1){
        fprintf(2, "Missing arguments");
        exit(0);
    }
    char buf[1024];
    char *exercArgs[MAXARG];
    int offset = 0;
    int i;
    char *p = buf;
    for(i = 1; i < argc; i++){
        exercArgs[i-1] = argv[i];
    }

    i--;
    char c;
    while(read(0, &c, sizeof(char)) != 0){
        if((c == ' ') || (c == '\t')){
            buf[offset++] = 0;
            exercArgs[i++] = p;
            p = buf + offset;
        }
        else if(c == '\n'){
            buf[offset++] = '\0';
            exercArgs[i] = p;
            p = buf + offset;
            
            if(fork() == 0){
                exec(exercArgs[0], exercArgs);
                exit(0);
            }
            wait(0);
            i=argc-1;
        }
        else{
            buf[offset++] = c;
        }
        
    }
    exit(0);
}