/*
* echoc .c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sockop.h"
#include <signal.h>
#include <sys/wait.h>
#include <curses.h>

int connfd ; /* socket descriptor */
void handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    close(connfd);
}

#define BUFSIZE 10
#include <stdio.h> 
#include <termios.h> 
#include <unistd.h> 
#include <fcntl.h> 
int kbhit(void) 
{ 
    struct termios oldt, newt; 
    int ch; 
    int oldf; 
    tcgetattr(STDIN_FILENO, &oldt); 
    newt = oldt; 
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); 
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0); 
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); 
    ch = getchar(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
    fcntl(STDIN_FILENO, F_SETFL, oldf); 
    if(ch != EOF) { 
        ungetc(ch, stdin); 
        return 1; 
    } 
    return 0; 
} 

int main (int argc, char * argv[ ])
{
    
    int n ;
    char buf[BUFSIZE]={0} ;
    char rcv[BUFSIZE]={0} ;
    int amount = 0 ;
    int amount_temp = 0 ;
    int amount_dig=0;
    int number = 0 ;
    int action_flag;  // deposit = 0 | withdraw = 1
    int i ;

    if(argc!=3)
        errexit("Usage : %s host_address host_port \n", argv[0]);
    signal(SIGCHLD, handler);

    /* create socket and connect to server */
    connfd = connectsock(argv[1], argv[2], "tcp") ;
    char cmd;
    while (1)
    {
        char send_msg[255];
        while(!kbhit()) ;
        cmd = getchar();
        // printf("Key %c\n", cmd);
        // sleep(2);
        if(cmd == 'w' || cmd == 'W')  // LEFT a
        {
            sprintf(send_msg, "W");
            printf("key %s\n",send_msg);
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");
        }
        else if(cmd == 'a' || cmd == 'A')  //UP w
        {
            sprintf(send_msg, "A");
            printf("key %s\n",send_msg);
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");
        }
        else if(cmd == 's' || cmd == 'S')  //RIGHT d
        {
            sprintf(send_msg, "S");
            printf("key %s\n",send_msg);
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");
        }
        else if(cmd == 'd' || cmd == 'D') //DOWN s
        {
            sprintf(send_msg, "D");
            printf("key %s\n",send_msg);
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");
        }
        napms(10);
    }
    
    close(connfd) ;
    return 0 ;
}
