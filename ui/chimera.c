#include <stdio.h>

#define BUFFMAX 100
int main(void) {

    char buff[BUFFMAX];

    // startup
    if (!fgets(buff, BUFFMAX, stdin)) {
        return 1;
    }
    buff[BUFFMAX] = 0;

    if (!strcmp(buff, "cmd")) {

        while(1){
            fprintf(stdout, "jess");
            usleep(1000000);
        }

    } else {

        while(1){
            fprintf(stdout, "test");
            usleep(1000000);
        }
    }
}
