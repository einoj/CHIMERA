#include <stdio.h>
#include <stdlib.h>
#define BUFFMAX 100

//int main() {
//    char buffer[BUFFMAX + 1];
//    char *bp = buffer;
//    int c;
//    FILE *in;
//    while (EOF != (c = fgetc(stdin)) && (bp - buffer) < BUFFMAX) {
//        *bp++ = c;
//    }
//    *bp = 0;    // Null-terminate the string
//    printf("%s", buffer);
//}

int main(void) {
    FILE *in;
    char buff[1024];

//#ifdef WIN32
    if(!(in = _popen("chimera.exe", "w"))){
        printf("Error %d: Can't open ./chimera.exe",in);
        return 1;
    }
//#elif UNIX
//    if(!(in = popen("chimera.exe", "rw"))){
//        printf("Error %d: Can't open ./chimera.exe",in);
//        return 1;
//    }
//#endif

    while(1){
        char buffer[] = { 'c','m','d', '\0' };
        //fwrite(buffer, sizeof(char), sizeof(buffer), in);

        fprintf(in,"%s",  buffer);
        fgets(buff,BUFFMAX, in);
        buff[100] = 0;
        
        printf("%s\n",buff);
        usleep(1000000);
    }
}
