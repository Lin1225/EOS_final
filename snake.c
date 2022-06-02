#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"
#include <signal.h>
#include <sys/time.h>
#include <pthread.h> 
#include "sockop.h"
#define NUM 60
#define COL 15
#define ROW 15
#define BUFFSIZE 256

struct direct
{
    int cx;
    int cy;
};
typedef struct node   
{
    int cx;
    int cy;
    struct node *back;
    struct node *next;
}node;
 
int mainMenu();
void initGame();       
void initGame2p();      
int setTicker(int );           
void show();  
void show2p();               
void showInformation();        
void showSnake();    
void showSnake2p();        
void getOrder();    
void *getOrder2p();         
void over(int i);     
void over2p(int i);      
 
void creatLink();              
void insertNode(int x, int y);
void deleteNode();
void creatLink2p();              
void insertNode2p(int x, int y);
void deleteNode2p();
void deleteLink();
void deleteLink2p();
void *cmdRead(void *arg);
 
int ch;
int fd;
unsigned short key;
lcd_write_info_t display;
int hour, minute, second;
int second_raw = 0;
int length, tTime, level, length2p;
int tick_timer = 1000;
int change_ticker = 0;
int change_ticker2p = 0;
int game_over = 0;
int overMode = 0;
int sockfd,client_fd;
struct direct dir, food, dir2p;
node *head, *tail;   
node *tmp_snake, *tmp2_snake;
node *head2p, *tail2p; 
int rank[10]={0};
pthread_t thread[50];
int thread_num = 0;
pthread_t ctrl_thread;

char send_msg[BUFFSIZE], read_msg[BUFFSIZE];

void sock_handler(int signum) {
    close(sockfd);
}
 
int main(int argc , char *argv[])
{
    signal(SIGINT, sock_handler);
    head = (node *)malloc( sizeof(node) );
    tail = (node *)malloc( sizeof(node) );
    head2p = (node *)malloc( sizeof(node) );
    tail2p = (node *)malloc( sizeof(node) );

    /*** Socket ***/
    struct sockaddr_in addr_cln;
    socklen_t sLen = sizeof ( addr_cln ) ;
    if(argc != 2)
        errexit("Usage : %s port \n" ,argv[0]);
    sockfd = passivesock(argv[1],"tcp");
    if(listen(sockfd, 5) == 0)
        printf("Listening\n");
    else if(listen(sockfd, 5) == -1)
        errexit("Error : listen()\n");

    /*** IO ***/
    if((fd = open("/dev/lcd", O_RDWR)) == -1) {
        perror("/dev/lcd");
        exit(EXIT_FAILURE);
    }
    ioctl(fd, KEY_IOCTL_CLEAR, key);
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    printf("Welcome to snake game\n");
    ioctl(fd, LCD_IOCTL_CUR_OFF, NULL);

    while(1){
        ioctl(fd, LCD_IOCTL_CLEAR, NULL);
        int game_opt = mainMenu();
        
        if(game_opt == 49){
            game_over = 0;
            overMode = 0;
            change_ticker = 0;
            initGame();
            signal(SIGALRM, show);
            getOrder();
            deleteLink();
        }

        else if(game_opt == 50){
            game_over = 0;
            overMode = 0;
            change_ticker = 0;
            change_ticker2p = 0;

            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char*)display.Msg, "Waiting for \nconnection...");
            ioctl(fd, LCD_IOCTL_WRITE, &display);

            client_fd = accept(sockfd, (struct sockaddr *) &addr_cln ,&sLen);
            if(client_fd == -1)
                errexit("Error : accept()\n");
            else{
                printf("New connection\n");
                pthread_create(&thread[thread_num], NULL, cmdRead, (void*) client_fd);
                thread_num++;
            }
            // getchar();
            initGame2p();
            signal(SIGALRM, show2p);
            // getOrder2p();
            pthread_create(&ctrl_thread, NULL, getOrder2p, NULL);
            while(game_over == 0){}
            deleteLink();
            deleteLink2p();
            // getOrder2p();
        }  

        else if(game_opt == 51){
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            showranking();
        }  
        else if(game_opt == 52){
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            // deleteLink(); 
            free(head);
            free(head2p);
            free(tail);
            free(tail2p);
            break;
        }       
    }

    
    
    

    initGame();
    signal(SIGALRM, show);
    getOrder();
    // endwin();
    return 0;
}

int mainMenu()
{
    display.CursorX = 0;
    display.CursorY = 2;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "Snake Legendary");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 7;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "1. Single");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 8;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "2. Double");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 9;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "3. Leader Board");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 10;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "4. Exit");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    key = key & 0xff;

    return key;
}

void showranking()
{
    int buf=0;
    int ii,j,p;
    rank[10]=length;
    for(ii=0;ii<11;ii++){
        for( j=ii;j<11;j++ ){
            if(rank[j]>rank[ii]){
                buf=rank[j];
                rank[j]=rank[ii];
                rank[ii]=buf;
            }
        }
    }
    for(p=0;p<10;p++){
        display.Count = sprintf((char*)display.Msg, "No.%d: %d\n",p, rank[p]);
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }
    ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
}

void *cmdRead(void *arg)
{
    int tmpMode;
    
    while(game_over == 0){
        memset(read_msg, 0, BUFFSIZE);
        if(read(client_fd, read_msg, BUFFSIZE) == -1)
            printf("Error : read()\n");

        if(strstr(read_msg, "UP")){
            dir2p.cx = -1;
            dir2p.cy = 0;
        }
        if(strstr(read_msg, "DOWN")){
            dir2p.cx = 1;
            dir2p.cy = 0;
        }
        if(strstr(read_msg, "LEFT")){
            dir2p.cx = 0;
            dir2p.cy = -1;
        }
        if(strstr(read_msg, "RIGHT")){
            dir2p.cx = 0;
            dir2p.cy = 1;
        }

        if(strstr(read_msg, "GameOver")){
            char tmp[256];
            sscanf(read_msg, "%s%d", tmp, &tmpMode);
            overMode = tmpMode;
            // over2p(overMode);
            printf("Game over %d\n", overMode);
        }
    }

    pthread_exit(NULL);
    close(client_fd);
}
 
void initGame()
{
    srand(time(0));
    tick_timer = 1000;

    hour = minute = second = tTime = 0;
    length = 1;
    dir.cx = 1;
    dir.cy = 0;
    ch = 'A';
    food.cy = rand() % COL;
    food.cx = rand() % (ROW-3) + 3;
    creatLink();
    setTicker(tick_timer); 
}

void initGame2p()
{
    srand(time(0));
    tick_timer = 1000;

    hour = minute = second = tTime = 0;

    length = 1;
    length2p = 1;
    
    dir.cx = 1;
    dir.cy = 0;
    
    dir2p.cx = -1;
    dir2p.cy = 0;
    
    ch = 'A';
    food.cy = 7;
    food.cx = 7;


    sprintf(send_msg, "food %d %d", food.cx, food.cy);
    if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");

    creatLink2p();
    setTicker(tick_timer); 
}
 
int setTicker(int m_secs)
{
    struct itimerval value, ovalue;
    int sec = m_secs/1000;
    int usec = (m_secs%1000)*1000;

    value.it_value.tv_sec = sec;
    value.it_value.tv_usec = usec;
    value.it_interval.tv_sec = sec;
    value.it_interval.tv_usec = usec;
    setitimer(ITIMER_REAL, &value, &ovalue);

    return 1;
}
 
void showInformation()
{
    tTime++;
    display.CursorX = 0;
    display.CursorY = 0;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "time: %d:%d:%d", hour, minute, second);
    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 1;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "length: %d", length);
    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 2;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "===============");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    // if(tTime*tick_timer >= 1000){
    //     second++;
    //     tTime = 0;
    // }
    second = (tTime*tick_timer)/1000;
    
    if(second >= NUM)
    {
        tTime = 0;
        second = 0;
        minute++;
    }
    if(minute >= NUM)
    {
        tTime = 0;
        minute = 0;
        hour++;
    }

}

void showInformation2p()
{
    tTime++;
    display.CursorX = 0;
    display.CursorY = 0;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "time: %d:%d:%d", hour, minute, second);
    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 1;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "length: %d/%d", length, length2p);
    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    display.CursorX = 0;
    display.CursorY = 2;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "===============");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    // if(tTime*tick_timer >= 1000){
    //     second++;
    //     tTime = 0;
    // }
    second = (tTime*tick_timer)/1000;
    second_raw = (tTime*tick_timer)/1000;
    
    if(second >= NUM)
    {
        tTime = 0;
        second = 0;
        minute++;
    }
    if(minute >= NUM)
    {
        tTime = 0;
        minute = 0;
        hour++;
    }

}

void showSnake()
{

    int lenChange = 0;
    int i;
    tmp_snake = head;

    display.CursorX = food.cy;
    display.CursorY = food.cx;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "$");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    if((COL-1==head->next->cy && 1==dir.cy)
        || (0==head->next->cy && -1==dir.cy)
        || (ROW-1==head->next->cx && 1==dir.cx)
        || (3==head->next->cx && -1==dir.cx))
    {
        over(1);
        return;
    }
    for(i = 0; i < length; i++){
        if( ((head->next->cy+dir.cy) == (tmp_snake->next->cy)) && ((head->next->cx+dir.cx) == (tmp_snake->next->cx)) ){
            over(2);
            // return;
        }
        else{
            tmp_snake = tmp_snake->next;
        }
    }
    tmp_snake = head;
    insertNode(head->next->cx+dir.cx, head->next->cy+dir.cy);

    if(head->next->cx==food.cx && head->next->cy==food.cy)
    {
        lenChange = 1;
        length++;
        if(length >= 50)
        {
            over(3);
            // return;
        }

        food.cy = rand() % COL;
        food.cx = rand() % (ROW-3) + 3;
        change_ticker = 0;
    }
    if(lenChange == 0)
    {
        // move(tail->back->cy, tail->back->cx);
        display.CursorX = tail->back->cy;
        display.CursorY = tail->back->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        // printw(" ");
        display.Count = sprintf((char*)display.Msg, " ");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        deleteNode();
    }
    // move(head->next->cy, head->next->cx);
    // tmp = tmp_snake->next;
    for(i = 0; i < length; i++){
        
        display.CursorX = tmp_snake->next->cy;
        display.CursorY = tmp_snake->next->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        display.Count = sprintf((char*)display.Msg, "*");
        ioctl(fd, LCD_IOCTL_WRITE, &display);

        tmp_snake = tmp_snake->next;

    }

}

void showSnake2p()
{

    int lenChange = 0;
    int lenChange2p = 0;
    int i;
    int eat;
    tmp_snake = head;
    tmp2_snake = head2p;

    display.CursorX = food.cy;
    display.CursorY = food.cx;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "$");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    if((COL-1==head->next->cy && 1==dir.cy)
        || (0==head->next->cy && -1==dir.cy)
        || (ROW-1==head->next->cx && 1==dir.cx)
        || (3==head->next->cx && -1==dir.cx))
    {
        over2p(1);
        // return;
    }

    tmp_snake = head;
    tmp2_snake = head2p;
    for(i = 0; i < length; i++){
        if( ((head->next->cy+dir.cy) == (tmp_snake->next->cy)) && ((head->next->cx+dir.cx) == (tmp_snake->next->cx)) ){
            over2p(2);
            // return;
        }
        else{
            tmp_snake = tmp_snake->next;
        }
    }

    tmp_snake = head;
    tmp2_snake = head2p;
    for(i = 0; i < length; i++){
        if( ((head2p->next->cy) == (tmp_snake->next->cy)) && ((head2p->next->cx) == (tmp_snake->next->cx)) ){
            over2p(4);
            // return;
        }
        else{
            tmp_snake = tmp_snake->next;
        }
    }

    tmp_snake = head;
    tmp2_snake = head2p;
    for(i = 0; i < length2p; i++){
        if( ((head->next->cy) == (tmp2_snake->next->cy)) && ((head->next->cx) == (tmp2_snake->next->cx)) ){
            over2p(4);
            // return;
        }
        else{
            tmp2_snake = tmp2_snake->next;
        }
    }



    tmp_snake = head;
    tmp2_snake = head2p;
    insertNode(head->next->cx+dir.cx, head->next->cy+dir.cy);
    insertNode2p(head2p->next->cx+dir2p.cx, head2p->next->cy+dir2p.cy);

    if(head->next->cx==food.cx && head->next->cy==food.cy)
    {
        lenChange = 1;
        length++;
        if(length >= 50)
        {
            over2p(3);
            // return;
        }

        food.cy = rand() % COL;
        food.cx = rand() % (ROW-3) + 3;
        change_ticker = 0;
        eat = 1;

        sprintf(send_msg, "food %d %d %d", food.cx, food.cy, eat);
        if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                    errexit("Error : write()\n");
    }

    if(head2p->next->cx==food.cx && head2p->next->cy==food.cy)
    {
        lenChange2p = 1;
        length2p++;
        if(length2p >= 50)
        {
            over2p(3);
            // return;
        }

        food.cy = rand() % COL;
        food.cx = rand() % (ROW-3) + 3;
        change_ticker2p = 0;
        eat = 2;

        sprintf(send_msg, "food %d %d %d", food.cx, food.cy, eat);
        if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                    errexit("Error : write()\n");
    }

    if(lenChange == 0)
    {
        // move(tail->back->cy, tail->back->cx);
        display.CursorX = tail->back->cy;
        display.CursorY = tail->back->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        // printw(" ");
        display.Count = sprintf((char*)display.Msg, " ");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        deleteNode();
    }
    if(lenChange2p == 0)
    {
        // move(tail->back->cy, tail->back->cx);
        display.CursorX = tail2p->back->cy;
        display.CursorY = tail2p->back->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        // printw(" ");
        display.Count = sprintf((char*)display.Msg, " ");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        deleteNode2p();
    }
    tmp_snake = head;
    tmp2_snake = head2p;
    // move(head->next->cy, head->next->cx);
    // tmp = tmp_snake->next;
    for(i = 0; i < length; i++){
        
        display.CursorX = tmp_snake->next->cy;
        display.CursorY = tmp_snake->next->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        display.Count = sprintf((char*)display.Msg, "*");
        ioctl(fd, LCD_IOCTL_WRITE, &display);

        tmp_snake = tmp_snake->next;
    }
    for(i = 0; i < length2p; i++){
        
        display.CursorX = tmp2_snake->next->cy;
        display.CursorY = tmp2_snake->next->cx;
        ioctl(fd, LCD_IOCTL_CUR_SET, &display);

        display.Count = sprintf((char*)display.Msg, "@");
        ioctl(fd, LCD_IOCTL_WRITE, &display);

        tmp2_snake = tmp2_snake->next;
    }

}
 
void show()
{
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    if((length % 3) == 0 && change_ticker == 0){
        tick_timer -= 200;
        if(tick_timer <= 100)
            tick_timer = 100;
        setTicker(tick_timer);
        change_ticker = 1;
    }

    showSnake();
    showInformation();


}

void show2p()
{
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    if((length % 3) == 0 && change_ticker == 0){
        tick_timer -= 200;
        if(tick_timer <= 100)
            tick_timer = 100;
        setTicker(tick_timer);
        change_ticker = 1;

    }
    if((length2p % 3) == 0 && change_ticker2p == 0){
        tick_timer -= 200;
        if(tick_timer <= 100)
            tick_timer = 100;
        setTicker(tick_timer);
        change_ticker2p = 1;
    }
    sprintf(send_msg, "refresh %d", second_raw);
    if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
        errexit("Error : write()\n");

    showSnake2p();
    showInformation2p();


    if(overMode != 0){
        setTicker(0); 
        over2p(overMode);
        
    }
}
 
void getOrder()
{
    int ret;
    unsigned short cmd;
    
    while(game_over == 0)
    {
        ioctl(fd, KEY_IOCTL_CLEAR, cmd);
        ret = ioctl(fd, KEY_IOCTL_WAIT_CHAR, &cmd);
        cmd = cmd & 0xff;
        printf("Key %d\n", cmd);
        if(cmd == 52)
        {
            dir.cx = 0;
            dir.cy = -1;
        }
        else if(cmd == 50)
        {
            dir.cx = -1;
            dir.cy = 0;
        }
        else if(cmd == 54)
        {
            dir.cx = 0;
            dir.cy = 1;
        }
        else if(cmd == 56)
        {
            dir.cx = 1;
            dir.cy = 0;
        }

    }
}

void *getOrder2p()
{
    int ret;
    unsigned short cmd;
    
    while(game_over == 0)
    {
        ret = ioctl(fd, KEY_IOCTL_WAIT_CHAR, &cmd);
        cmd = cmd & 0xff;
        // printf("Cmd %d\n", cmd);
        if(cmd == 52)
        {
            dir.cx = 0;
            dir.cy = -1;

            sprintf(send_msg, "LEFT");
            if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 50)
        {
            dir.cx = -1;
            dir.cy = 0;

            sprintf(send_msg, "UP");
            if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 54)
        {
            dir.cx = 0;
            dir.cy = 1;

            sprintf(send_msg, "RIGHT");
            if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 56)
        {
            dir.cx = 1;
            dir.cy = 0;

            sprintf(send_msg, "DOWN");
            if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }


    }
    pthread_exit(NULL);
}
 
void over(int i)
{

    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.CursorX = 0;
    display.CursorY = 0;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);


    if(1 == i){
        display.Count = sprintf((char*)display.Msg, "Crash the wall.\nGame over\n\nPress any key \nto continue");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(2 == i){
        display.Count = sprintf((char*)display.Msg, "Crash itself.\nGame over\n\nPress any key \nto continue");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(3 == i){
        display.Count = sprintf((char*)display.Msg, "Mission Complete\n\nPress any key \nto continue");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }

    setTicker(0);             
    // deleteLink();            
    // exit(0);
    game_over = 1;
}

void over2p(int i)
{

    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    display.CursorX = 0;
    display.CursorY = 0;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);


    if(1 == i){
        display.Count = sprintf((char*)display.Msg, "Crash the wall.\nGame over");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                    errexit("Error : write()\n");
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(2 == i){
        display.Count = sprintf((char*)display.Msg, "Crash itself.\nGame over");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                    errexit("Error : write()\n");
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(3 == i){
        display.Count = sprintf((char*)display.Msg, "Mission Complete");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        sprintf(send_msg, "GameOver %d", i);
        if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
                    errexit("Error : write()\n");
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(4 == i){
        display.Count = sprintf((char*)display.Msg, "Snake bite each.\nGame over");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    display.CursorX = 0;
    display.CursorY = 4;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    if(length > length2p){
        display.Count = sprintf((char*)display.Msg, "1p win");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }
    else if(length < length2p){
        display.Count = sprintf((char*)display.Msg, "2p win");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }
    else if(length == length2p){
        display.Count = sprintf((char*)display.Msg, "tie");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }

    // sprintf(send_msg, "GameOver %d", i);
    // if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
    //             errexit("Error : write()\n");

    setTicker(0); 
    game_over = 1;

    display.CursorX = 0;
    display.CursorY = 12;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "Press any key \nto continue");
    ioctl(fd, LCD_IOCTL_WRITE, &display);
    ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);

                
    // deleteLink();            
    // exit(0);
    
}
 
void creatLink()
{
    node *temp = (node *)malloc( sizeof(node) );
    temp->cx = 3;
    temp->cy = 5;
    head->back = tail->next = NULL;
    head->next = temp;
    temp->next = tail;
    tail->back = temp;
    temp->back = head;
}

void creatLink2p()
{
    node *temp = (node *)malloc( sizeof(node) );
    node *temp2 = (node *)malloc( sizeof(node) );
    
    temp->cx = 3;
    temp->cy = 5;
    head->back = tail->next = NULL;
    head->next = temp;
    temp->next = tail;
    tail->back = temp;
    temp->back = head;

    
    temp2->cx = 11;
    temp2->cy = 9;
    head2p->back = tail2p->next = NULL;
    head2p->next = temp2;
    temp2->next = tail2p;
    tail2p->back = temp2;
    temp2->back = head2p;
}
 
void insertNode(int x, int y)
{
    node *temp = (node *)malloc( sizeof(node) );
    temp->cx = x;
    temp->cy = y;
    temp->next = head->next;
    head->next = temp;
    temp->back = head;
    temp->next->back = temp;
}

void insertNode2p(int x, int y)
{
    node *temp = (node *)malloc( sizeof(node) );
    temp->cx = x;
    temp->cy = y;
    temp->next = head2p->next;
    head2p->next = temp;
    temp->back = head2p;
    temp->next->back = temp;
}
 
void deleteNode()
{
    node *temp = tail->back;
    node *bTemp = temp->back;
    bTemp->next = tail;
    tail->back = bTemp;
    temp->next = temp->back = NULL;
    free(temp);
    temp = NULL;
}

void deleteNode2p()
{
    node *temp = tail2p->back;
    node *bTemp = temp->back;
    bTemp->next = tail2p;
    tail2p->back = bTemp;
    temp->next = temp->back = NULL;
    free(temp);
    temp = NULL;
}
 
void deleteLink()
{
    while(head->next != tail)
        deleteNode();
    head->next = tail->back = NULL;
    // free(head);
    // free(tail);
}

void deleteLink2p()
{
    while(head2p->next != tail2p)
        deleteNode2p();
    head2p->next = tail2p->back = NULL;
    // free(head2p);
    // free(tail2p);
}
