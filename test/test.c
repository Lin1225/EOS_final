// /**
//  Linux (POSIX) implementation of _kbhit().
//  Morgan McGuire, morgan@cs.brown.edu
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/select.h>
// #include <termios.h>
// #include "stdbool.h"
// #include <sys/ioctl.h> //不加这个头文件会导致FIONREAD未定义的错误
 
// int _kbhit() {
//     static const int STDIN = 0;
//     static bool initialized = false;
 
//     if (! initialized) {
//         // Use termios to turn off line buffering
//         struct termios term;
//         tcgetattr(STDIN, &term);
//         term.c_lflag &= ~ICANON;
//         tcsetattr(STDIN, TCSANOW, &term);
//         setbuf(stdin, NULL);
//         initialized = true;
//     }
 
//     int bytesWaiting;
//     ioctl(STDIN, FIONREAD, &bytesWaiting);
//     return bytesWaiting;
// }
 
// //
// //    Simple demo of _kbhit()
 
// #include <unistd.h>
 
// int main(int argc, char** argv) {
//     printf("Press any key");
//     while (! _kbhit()) {
//         printf(".");
//         fflush(stdout);
//         usleep(1000);
//     }
//     printf("\nDone.\n");
 
//     return 0;
// } 


// #include <stdio.h> 
// #include <termios.h> 
// #include <unistd.h> 
// #include <fcntl.h> 
 
// int kbhit(void) 
// { 
//     struct termios oldt, newt; 
//     int ch; 
//     int oldf; 
//     tcgetattr(STDIN_FILENO, &oldt); 
//     newt = oldt; 
//     newt.c_lflag &= ~(ICANON | ECHO); 
//     tcsetattr(STDIN_FILENO, TCSANOW, &newt); 
//     oldf = fcntl(STDIN_FILENO, F_GETFL, 0); 
//     fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); 
//     ch = getchar(); 
//     tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
//     fcntl(STDIN_FILENO, F_SETFL, oldf); 
//     if(ch != EOF) { 
//         ungetc(ch, stdin); 
//         return 1; 
//     } 
//     return 0; 
// } 
 
// int main(void) 
// { 
// while(!kbhit()) ; 
// printf("You pressed '%c'!\n", getchar()); 
// return 0; 
// }

#include <stdio.h> 
#include <stdlib.h>
 
int main(void) 
{ 
    int** a = (int ** )malloc(sizeof(int*)*2);
    *a = (int*)malloc(sizeof(int)*1);
    int *b = (int*)malloc(sizeof(int)*3);
    *(b+2) = 3;
    b[0] = 2;
    a[0] = b;
    printf("%d\n",b[2]);
}