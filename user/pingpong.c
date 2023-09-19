#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"


int main(int argc, char *argv[]){
    int p[2];
    int s[2];
    pipe(p);
    pipe(s);
    char buf[1];
    if(fork()==0){
        close(p[1]);
        close(s[0]);
        if(read(p[0], buf, sizeof buf) > 0){
            fprintf(1,"%d: received ping\n", getpid());
            write(s[1], buf, sizeof buf);
        }
        exit(0);
    }
    close(p[0]);
    close(s[1]);
    write(p[1], buf, sizeof buf);
    if(read(s[0], buf, sizeof buf) > 0){
        fprintf(1,"%d: received pong\n", getpid());
    }
    exit(0);
}