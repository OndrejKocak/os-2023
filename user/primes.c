#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void primeSieve(int p[]){
    int forkRet = fork();
        if(forkRet == 0){
            int prime = 0;
            int n;
            if(read(p[0], &prime, sizeof(int)) == 0){
                exit(0);
            }
            fprintf(1, "prime %d\n", prime);
            int tmp[2];
            pipe(tmp);
            while(read(p[0], &n, sizeof(int))){
                if(n%prime != 0){
                    write(tmp[1], &n, sizeof(int));
                }
            }
            close(p[0]);
            close(tmp[1]);
            primeSieve(tmp);
        }
        
        
        close(p[0]);
        wait(0);
}

int main(int argc, char *argv[]){
    int p[2];
    pipe(p);
    for(int j = 2; j <= 35; j++){
        write(p[1], &j, sizeof(int));
    }
    close(p[1]);
    primeSieve(p);
    exit(0);
}