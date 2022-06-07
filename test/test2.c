
#include <curses.h>
#include <ncurses.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
// #include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"
#include <signal.h>
#include <sys/time.h>
#include <pthread.h> 
#include "sockop.h"

#define NUM 60
#define COL 20
#define ROW 30
#define BUFFSIZE 256

struct direct
{
    int cx;
    int cy;
    int type;
};
typedef struct node   
{
    int cx;
    int cy;
    struct node *back;
    struct node *next;
}node;
 
char mainMenu();
void initGame(node*,node*,int);       
void initGame2p();      
int setTicker(int );           
void show();  
void show2p();               
void showInformation();        
void showSnake();    
void showSnake2p();        
void getOrder();    
void *getOrder2p(void* tid,void* id); 
void getOrder2p_test(int connfd,int snake_id);        
void over(int i);     
void over2p(int i); 
 
void creatLink(node* head,node* tail);              
void insertNode(node *snake,int x, int y);
void deleteNode(node *snake);
void creatLink2p();              
void insertNode2p(int x, int y);
void deleteNode2p();
void deleteLink(node *snake,node* snake_tail);
void deleteLink2p();
void *cmdRead(void *arg);
void tailinsertNode(node *snake );
void showranking();

int ch;
int fd;
unsigned short key;
int hour, minute, second;
int second_raw = 0;
int length[50], tTime, level, length2p;
int tick_timer = 1000;
int change_ticker = 0;
int change_ticker2p = 0;
int game_over = 0;
int overMode = 0;
int sockfd,client_fd;
struct direct dir[50], food, dir2p;
node** snake,**snake_tail;
node *head, *tail;   
node **tmp_snake;
node *tmp2_snake;
node *head2p, *tail2p; 
int rank[10]={0};
pthread_t thread[50];
int thread_num = 0;
pthread_t ctrl_thread;
int snake_client = 0;

char send_msg[BUFFSIZE], read_msg[BUFFSIZE];

enum food_type {normal, faster, bigger, slower};

void sock_handler(int signum) {
    endwin();
    exit(1);
}
 

void *create( void * tid){

    int connfd ;
    connfd = *(int *)tid;
    
    head = (node *)malloc( sizeof(node) );
    tail = (node *)malloc( sizeof(node) );
    
    if (snake_client==0)
    {
        snake = (node**)malloc(sizeof(node*)*50);
        *snake = (node*)malloc(sizeof(node));
        snake_tail = (node**)malloc(sizeof(node*)*50);
        *snake_tail = (node*)malloc(sizeof(node));
    }
    
    
    snake[snake_client] = head;
    snake_tail[snake_client] = tail;


    game_over = 0;
    overMode = 0;
    change_ticker = 0;
    initGame(*(snake+snake_client),*(snake_tail+snake_client),snake_client);
    snake_client ++ ;
    
    // getOrder();
    // pthread_create(&ctrl_thread, NULL, &getOrder2p, (void *) &connfd,(void*)& snake_client);
    int id = snake_client-1;
    getOrder2p_test(connfd,id);
    
}

int main(int argc , char *argv[])
{
    if(argc!=2)
        errexit("Usage : %s port \n", argv[0]);
   
    
    signal(SIGINT, sock_handler);
    // head = (node *)malloc( sizeof(node) );
    // tail = (node *)malloc( sizeof(node) );
    head2p = (node *)malloc( sizeof(node) );
    tail2p = (node *)malloc( sizeof(node) );
    tmp_snake = (node**)malloc(sizeof(node*));


    printf("Welcome to snake game\n");
    
    

    
    struct sockaddr_in addr_cln ;
    
    socklen_t sLen = sizeof(addr_cln) ;
        
    sockfd = passivesock(argv[1], "tcp", 10) ;

    char mode=0;
    int counter = 0;
    pthread_t threads[100]={0};
    int connfd[100]={0};
    int rc;
    int i = 0;

    initscr();
    cbreak();
    noecho();
    signal(SIGALRM, show);
    while(1){
        clear();
        refresh();
                
        connfd[i] = accept(sockfd,(struct sockaddr *) &addr_cln , &sLen );
        
        if(connfd[i] == -1){
            errexit("Error : accept() : %s\n",strerror(errno));
        }
        if(rc=pthread_create(&threads[i],NULL,&create,(void *) &connfd[i])<0){
            
            exit(-1);
        }
        i++;  
                
    }
    

    // endwin();
    return 0;
}

char mainMenu()
{
    // clear();
    mvaddstr(2, 0, "Snake Legendary");
    
    mvaddstr(7, 0, "1. Single");

    mvaddstr(8, 0, "2. Double");
    
    mvaddstr(9, 0, "3. Leader Board");
    
    mvaddstr(10, 0, "4. Exit");
    refresh();
    char a;
    a = getch();

    return a;
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
 
void initGame(node* head,node* tail,int snake_id)
{
    creatLink(head,tail);
    length[snake_id] = 1;
    dir[snake_id].cx = 1;
    dir[snake_id].cy = 0;
    if(snake_id==0){
        srand(time(0));
        tick_timer = 100;
        hour = minute = second = tTime = 0;
        ch = 'A';
        food.cy = rand() % (COL-3)  +3;
        food.cx = rand() % (ROW-1) ;
        setTicker(tick_timer); 
    }
    
}

int setTicker(int m_secs)
{
    if (m_secs==0);
    else if(m_secs<50){
        m_secs = 50;
    }
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
    // display.CursorX = 0;
    // display.CursorY = 0;
    // ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    // display.Count = sprintf((char*)display.Msg, "time: %d:%d:%d", hour, minute, second);
    // // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    // ioctl(fd, LCD_IOCTL_WRITE, &display);
    char aaaa[50];
    sprintf(aaaa,"time: %d:%d:%d", hour, minute, second);
    mvaddstr(0, 0, aaaa);

    // display.CursorX = 0;
    // display.CursorY = 1;
    // ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    // display.Count = sprintf((char*)display.Msg, "length: %d", length);
    // // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    // ioctl(fd, LCD_IOCTL_WRITE, &display);
    // char aaaa[50];
    for (int i = 0; i < snake_client; i++)
    {   
        int num = 0;
        num = sprintf(aaaa,"snake %d length: %d",i, length[i]);
        if(i==0)
            mvaddstr(1, 0, aaaa);
        else
            mvaddstr(1, num+1, aaaa);
    }
    
    

    // display.CursorX = 0;
    // display.CursorY = 2;
    // ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    // display.Count = sprintf((char*)display.Msg, "===============");
    // ioctl(fd, LCD_IOCTL_WRITE, &display);
    // char aaaa[50];
    sprintf(aaaa,"===============================");
    mvaddstr(2, 0, aaaa);
    
    mvaddstr(20, 0, aaaa);


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

int counter = 0;
void showSnake()
{
    // printf("It's %d times\n",counter++);
    char aaa[50];
    sprintf(aaa,"It's %d times\n",counter++);
    mvaddstr(0, 15, aaa);
    int kkkk = 0;
    for (kkkk = 0; kkkk < snake_client; kkkk++)
    {

        
        int lenChange = 0;
        int i;
        
        *tmp_snake = (*(snake+kkkk));
        
        mvaddstr(food.cy, food.cx, "$");
        
        // check head touch the arena
        if((COL-1==(*(snake+kkkk))->next->cy && 1==dir[kkkk].cy)
            || (3==(*(snake+kkkk))->next->cy && -1==dir[kkkk].cy)
            || (ROW-1==(*(snake+kkkk))->next->cx && 1==dir[kkkk].cx)
            || (0==(*(snake+kkkk))->next->cx && -1==dir[kkkk].cx))
        {
            over(1);
            return;
        }
        
        // check head touch its body
        for(i = 0; i < length[kkkk]; i++){
            if( (((*(snake+kkkk))->next->cy+dir[kkkk].cy) == ((*tmp_snake)->next->cy)) && (((*(snake+kkkk))->next->cx+dir[kkkk].cx) == ((*tmp_snake)->next->cx)) ){
                over(2);
                return;
            }
            else{                
                *tmp_snake = (*tmp_snake)->next;
            }
        }
        
        
        // check touch other snake
        for(i = 0;i < snake_client; i++){
            if(kkkk == i) continue;
            *tmp_snake = (*(snake+i));
            int j;
            for(j = 0; j < length[i]; j++)
                if( (((*(snake+kkkk))->next->cy+dir[kkkk].cy) == ((*tmp_snake)->next->cy)) && (((*(snake+kkkk))->next->cx+dir[kkkk].cx) == ((*tmp_snake)->next->cx)) ){
                    over(4);
                    return;
                }
                else{                
                    *tmp_snake = (*tmp_snake)->next;
                }
        }
        *tmp_snake = (*(snake+kkkk));
        // snake lenght +1 at first
        insertNode((*(snake+kkkk)),(*(snake+kkkk))->next->cx+dir[kkkk].cx, (*(snake+kkkk))->next->cy+dir[kkkk].cy);
        
        // check get food or not
        if((*(snake+kkkk))->next->cx==food.cx && (*(snake+kkkk))->next->cy==food.cy){
            lenChange = 1;
            length[kkkk]++;
            if(length[kkkk] >= 20)
            {
                over(3);
                game_over = 1;
                return;
            }
            switch (food.type)
            {
                case normal:
                    // printf("It's normal guy\n");
                    break;
                case faster:
                    tick_timer -=200;
                    // printf("faster faster\n");
                    setTicker(tick_timer);
                    break;
                case  bigger:
                    // printf("big gun\n");
                    length[kkkk]++;
                    tailinsertNode(*(snake_tail+kkkk));
                    break;
                case slower:
                    // printf("old man\n");
                    tick_timer +=200;
                    setTicker(tick_timer);
                    break;
                default:
                    break;
            }

            food.cy = rand() % (COL-3)  +3;
            food.cx = rand() % (ROW-1) ;
            food.type = rand() % 4;
            change_ticker = 0;
        }
        
        if(lenChange == 0)
        {
            // move(tail->back->cy, tail->back->cx);
            // display.CursorX = tail->back->cy;
            // display.CursorY = tail->back->cx;
            // ioctl(fd, LCD_IOCTL_CUR_SET, &display);
            // // printw(" ");
            // display.Count = sprintf((char*)display.Msg, " ");
            // ioctl(fd, LCD_IOCTL_WRITE, &display);
            
            mvaddstr((*(snake_tail+kkkk))->back->cy, (*(snake_tail+kkkk))->back->cx, " ");
            
            deleteNode(*(snake_tail+kkkk));
            
        }
        // move(head->next->cy, head->next->cx);
        // tmp = tmp_snake->next;
       
        for(i = 0; i < length[kkkk]; i++){
            
            // display.CursorX = tmp_snake->next->cy;
            // display.CursorY = tmp_snake->next->cx;
            // ioctl(fd, LCD_IOCTL_CUR_SET, &display);
            // display.Count = sprintf((char*)display.Msg, "*");
            // ioctl(fd, LCD_IOCTL_WRITE, &display);
            mvaddstr((*tmp_snake)->next->cy, (*tmp_snake)->next->cx, "*");
            
            *tmp_snake = (*tmp_snake)->next;

        }
        
    }
    
    

}

void send_arround(int connfd, char array[255],char centerx,char centery,char reward){
    char *p = (char*)malloc(sizeof(char)*255+sizeof(char) * 4);
    memcpy(p,array,255);
    p[255] = centerx;
    p[255+1] = centery;
    p[255+2] = reward; // time clock, length, nothing
    p[255+3] = 1;

    // write to socket
    if(write(connfd, p, sizeof(p)) == -1)
        // errexit("Error : write()\n");
        perror("Error : write()\n");

}

void show()
{
    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    // clear();
    // refresh();
    if((length[0] % 3) == 0 && change_ticker == 0){
        tick_timer -= 200;
        if(tick_timer <= 100)
            tick_timer = 100;
        setTicker(tick_timer);
        change_ticker = 1;
    }

    showSnake();
    if(game_over==0)showInformation();
    refresh();
}

void getOrder()
{
    int ret;
    char cmd;
    // char ch;
    
    while(game_over == 0)
    {
        // ioctl(fd, KEY_IOCTL_CLEAR, cmd);
        // ret = ioctl(fd, KEY_IOCTL_WAIT_CHAR, &cmd);
        // cmd = cmd & 0xff;
        
        cmd = getch();
        // printf("Key %d\n", cmd);
        if(cmd == 'w' || cmd == 'W')  // LEFT a
        {
            dir[0].cx = 0;
            dir[0].cy = -1;
        }
        else if(cmd == 'a' || cmd == 'A')  //UP w
        {
            dir[0].cx = -1;
            dir[0].cy = 0;
        }
        else if(cmd == 's' || cmd == 'S')  //RIGHT d
        {
            dir[0].cx = 0;
            dir[0].cy = 1;
        }
        else if(cmd == 'd' || cmd == 'D') //DOWN s
        {
            dir[0].cx = 1;
            dir[0].cy = 0;
        }
        napms(10);
    }
}

void *getOrder2p(void* tid,void* id)
{
    int connfd ;
    connfd = *(int *)tid;

    int snake_id ;
    snake_id = *(int *)id;

    int ret;
    unsigned short cmd;
    char aaa[50]={0};
    sprintf(aaa,"game_over %d",game_over);
    printf("+++++++++++ %s\n",aaa);
    sprintf(aaa,"connfd %d",connfd);
    printf("+++++++++++ %s\n",aaa);
    
    while(game_over == 0)
    {
        // mvaddstr(9, 0, aaa);
        // mvaddstr(10, 0, "$$$$$$$$$$$$$");
        printf("$$$$$$$$$$$$$\n");
        // refresh();
        
        // sleep(5);
        char rcv[255]={0};
        if(read(connfd, rcv, 255) == -1)
            printf("Error : read()\n");
        
        printf("$$$$$$$$$$$$$\n");
        printf("Cmd %s\n", rcv);
        
        
        if(rcv[0]=='a' || rcv[0]=='A')
        {
            dir[snake_id].cx = 0;
            dir[snake_id].cy = -1;

        }
        else if(rcv[0]=='W' || rcv[0]=='w')
        {
            dir[snake_id].cx = -1;
            dir[snake_id].cy = 0;
        }
        else if(rcv[0]=='d' || rcv[0]=='D')
        {
            dir[snake_id].cx = 0;
            dir[snake_id].cy = 1;
        }
        else if(rcv[0]=='s' || rcv[0]=='S')
        {
            dir[snake_id].cx = 1;
            dir[snake_id].cy = 0;

        }


    }

    pthread_exit(NULL);
}

void getOrder2p_test(int connfd,int snake_id)
{
    int ret;
    unsigned short cmd;
    char aaa[50]={0};
    
    while(game_over == 0)
    {
        // mvaddstr(9, 0, aaa);
        // mvaddstr(10, 0, "$$$$$$$$$$$$$");
        // refresh();
        
        // sleep(5);
        char rcv[255]={0};
        if(read(connfd, rcv, 255) == -1)
            printf("Error : read()\n");
              
        
        if(rcv[0]=='w' || rcv[0]=='W')
        {
            dir[snake_id].cx = 0;
            dir[snake_id].cy = -1;

        }
        else if(rcv[0]=='a' || rcv[0]=='A')
        {
            dir[snake_id].cx = -1;
            dir[snake_id].cy = 0;
        }
        else if(rcv[0]=='s' || rcv[0]=='S')
        {
            dir[snake_id].cx = 0;
            dir[snake_id].cy = 1;
        }
        else if(rcv[0]=='d' || rcv[0]=='D')
        {
            dir[snake_id].cx = 1;
            dir[snake_id].cy = 0;

        }


    }

    pthread_exit(NULL);
}

void over(int i)
{

    // ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    // display.CursorX = 0;
    // display.CursorY = 0;
    // ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    game_over = 1;
    snake_client=0;
    clear();
    

    if(1 == i){
        // display.Count = sprintf((char*)display.Msg, "Crash the wall.\nGame over\n\nPress any key \nto continue");
        // ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
        mvaddstr(0, 0, "Crash the wall.\nGame over\n\nPress any key \nto continue");
    }
    else if(2 == i){
        // display.Count = sprintf((char*)display.Msg, "Crash itself.\nGame over\n\nPress any key \nto continue");
        // ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
        mvaddstr(0, 0, "Crash itself.\nGame over\n\nPress any key \nto continue");
    }
    else if(3 == i){
        // display.Count = sprintf((char*)display.Msg, "Mission Complete\n\nPress any key \nto continue");
        // ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
        mvaddstr(0, 0, "Mission Complete\n\nPress any key \nto continue");
    }
    else if(4 == i){
        // display.Count = sprintf((char*)display.Msg, "Mission Complete\n\nPress any key \nto continue");
        // ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
        mvaddstr(0, 0, "Crash other snake.\nGame over\n\nPress any key \nto continue");
    }

    setTicker(0);             
    // deleteLink();            
    // exit(0);
    refresh();
    
}

void creatLink(node* head,node* tail)
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


void insertNode(node* snakeX, int x, int y)
{
    node *temp = (node *)malloc( sizeof(node) );
    temp->cx = x;
    temp->cy = y;
    temp->next = snakeX->next;
    snakeX->next = temp;
    temp->back = snakeX;
    temp->next->back = temp;
}

void tailinsertNode(node* snake){
    node *temp = (node *)malloc( sizeof(node) );
    int dirx = snake->back->back->cx-snake->back->cx;
    int diry = snake->back->back->cy-snake->back->cy;
    temp->cx = snake->back->cx + dirx;
    temp->cy = snake->back->cy + diry;

    temp->back = snake->back;
    snake->back->next = temp;
    snake->back = temp;
    temp->next = snake;

} 

void deleteNode(node *snake)
{
    node *temp = snake->back;
    node *bTemp = temp->back;
    bTemp->next = snake;
    snake->back = bTemp;
    temp->next = temp->back = NULL;
    free(temp);
    temp = NULL;
}

 
void deleteLink(node *snake,node* snake_tail)
{
    while(snake->next != snake_tail)
        deleteNode(snake);
    snake->next = snake_tail->back = NULL;
    // free(head);
    // free(tail);
}


