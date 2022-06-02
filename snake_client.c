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
#include <semaphore.h> 
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
typedef struct info
{
    int rank;
    char name[10];
}info;
 
int mainMenu();
void initGame();       
void initGame2p();      
int setTicker(int );           
void show();  
void *show2p();               
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
void semReset();
 
int ch;
int fd;
unsigned short key;
lcd_write_info_t display;
int hour, minute, second;
int length, tTime, level, length2p;
int tick_timer = 1000;
int change_ticker = 0;
int change_ticker2p = 0;
int game_over = 0;
int connfd;
int overMode = 0;
int who_eat;
int second2p;
int new_round = 0;
struct direct dir, food, dir2p;
node *head, *tail;   
node *tmp_snake, *tmp2_snake;
node *head2p, *tail2p; 
info rank[10];
int length_info = 0;


pthread_t thread[50];
pthread_t ctrl_thread[50];
pthread_t show_thread[50];
char send_msg[BUFFSIZE], read_msg[BUFFSIZE];
int thread_num = 0;

sem_t food_sem;
sem_t refresh_sem;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void sock_handler(int signum) {
    close(connfd);
}
 
int main(int argc , char *argv[])
{
    signal(SIGINT, sock_handler);
    sem_init(&food_sem, 0, 0);
    // sem_init(&refresh_sem, 0, 0);
    pthread_mutex_init(&(mutex),NULL); 
    head = (node *)malloc( sizeof(node) );
    tail = (node *)malloc( sizeof(node) );
    head2p = (node *)malloc( sizeof(node) );
    tail2p = (node *)malloc( sizeof(node) );
    // memset(rank, 0, 11);
    /*** Socket ***/
    if(argc != 3)
        errexit("Usage : %s host address host port message n\n" ,argv[0]);
    
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

           rank[10].rank = 0;
           length_info = 0;
            memset(rank[10].name, '\0', 10);

            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char*)display.Msg, "Please enter \nyour name:\n");
            ioctl(fd, LCD_IOCTL_WRITE, &display);
            display.Count = sprintf((char*)display.Msg, "Press '#' \nto start.\n");
            ioctl(fd, LCD_IOCTL_WRITE, &display);


            while(1){
                ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
                key = key & 0xff;
                if(key == 65){
                    display.Count = sprintf((char*)display.Msg, "A");
                    ioctl(fd, LCD_IOCTL_WRITE, &display);
                    rank[10].name[length_info] = 'A';
                    length_info++;
                }
                if(key == 66){
                    display.Count = sprintf((char*)display.Msg, "B");
                    ioctl(fd, LCD_IOCTL_WRITE, &display);
                    rank[10].name[length_info] = 'B';
                    length_info++;
                }
                if(key == 67){
                    display.Count = sprintf((char*)display.Msg, "C");
                    ioctl(fd, LCD_IOCTL_WRITE, &display);
                    rank[10].name[length_info] = 'C';
                    length_info++;
                }
                if(key == 68){
                    display.Count = sprintf((char*)display.Msg, "D");
                    ioctl(fd, LCD_IOCTL_WRITE, &display);
                    rank[10].name[length_info] = 'D';
                    length_info++;
                }
                if(key == 35){
                    break;
                }
            }

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
            new_round = 0;

            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            display.Count = sprintf((char*)display.Msg, "Waiting for \nconnection...");
            ioctl(fd, LCD_IOCTL_WRITE, &display);

            connfd = connectsock(argv[1],argv[2],"tcp");
            pthread_create(&thread[thread_num], NULL, cmdRead, (void*) connfd);
            // getchar();
            initGame2p();
            // signal(SIGALRM, show2p);
            pthread_create(&ctrl_thread[thread_num], NULL, getOrder2p, NULL);
            pthread_create(&show_thread[thread_num], NULL, show2p, NULL);
            thread_num++;
            while(new_round == 0){}

            deleteLink();
            deleteLink2p();
            // sem_destroy(&refresh_sem);  
            // getOrder2p();
            // getOrder2p();
        }  

        else if(game_opt == 51){
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            showranking();
        }  
        else if(game_opt == 52){
            ioctl(fd, LCD_IOCTL_CLEAR, NULL);
            sem_destroy(&food_sem);
            sem_destroy(&refresh_sem);
            
            pthread_mutex_destroy(&mutex);
            // deleteLink(); 
            free(head);
            free(head2p);
            free(tail);
            free(tail2p);
            break;
        }  
           
    }

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

void *cmdRead(void *arg)
{
    int tmpMode;
    int food_x, food_y;
    int eat;
    int new_ticker;

    while(game_over == 0){
        memset(read_msg, 0, BUFFSIZE);
        if(read(connfd, read_msg, BUFFSIZE) == -1)
            printf("Error : read()\n");

        if(strstr(read_msg, "UP")){
            dir.cx = -1;
            dir.cy = 0;
            // printf("up\n");
        }
        if(strstr(read_msg, "DOWN")){
            dir.cx = 1;
            dir.cy = 0;
            // printf("down\n");
        }
        if(strstr(read_msg, "LEFT")){
            dir.cx = 0;
            dir.cy = -1;
            // printf("left\n");
        }
        if(strstr(read_msg, "RIGHT")){
            dir.cx = 0;
            dir.cy = 1;
            // printf("right\n");
        }
        if(strstr(read_msg, "refresh")){
            char tmp[256];
            sem_post(&refresh_sem);
            sscanf(read_msg, "%s%d", tmp, &second2p);
            // tick_timer = new_ticker;
        }

        if(strstr(read_msg, "food")){
            char tmp[256];
            sscanf(read_msg, "%s%d%d%d", tmp, &food_x, &food_y, &eat);
            // printf("Food x : %d, y : %d\n", food_x, food_y);
            food.cx = food_x;
            food.cy = food_y;
            pthread_mutex_lock(&mutex);
            who_eat = eat;
            pthread_mutex_unlock(&mutex);
            sem_post(&food_sem);
        }

        if(strstr(read_msg, "GameOver")){
            char tmp[256];
            sscanf(read_msg, "%s%d", tmp, &tmpMode);
            // if(overMode != 4){
                overMode = tmpMode;
                sem_post(&refresh_sem);

                printf("Game over %d\n", overMode);
            // }
            
            // over2p(overMode);
        }
    }

    pthread_exit(NULL);
    close(connfd);
}

void semReset()
{
    int sem_value;
    int i;
    // for(i = 0; i < 5; i++)
    //     sem_post(&refresh_sem);
    sem_getvalue(&refresh_sem, &sem_value);

    while(sem_value > 0){
        sem_wait(&refresh_sem);
        sem_getvalue(&refresh_sem, &sem_value);
    }
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
    // sem_init(&refresh_sem, 0, 0);
    semReset();
    // srand(time(0));
    tick_timer = 1000;
    second2p = 0;

    hour = minute = second = tTime = 0;

    length = 1;
    length2p = 1;
    
    dir.cx = 1;
    dir.cy = 0;
    
    dir2p.cx = -1;
    dir2p.cy = 0;
    
    ch = 'A';
    // food.cy = 7;
    // food.cx = 7;
    sem_wait(&food_sem);

    // sprintf(send_msg, "food %d %d", food.cx, food.cy);
    // if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
    //             errexit("Error : write()\n");

    creatLink2p();
    // setTicker(tick_timer); 
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
void showranking()
{
    info buf;
    int ii, j, p, num;
    rank[10].rank = length;
    for(ii = 0; ii < 11; ii++){
        for(j = ii; j < 11; j++){
            if(rank[j].rank > rank[ii].rank){
                buf = rank[j];
                rank[j] = rank[ii];
                rank[ii] = buf;
            }
        }
    }
    display.Count = sprintf((char*)display.Msg, "  Leader Board\n");
    ioctl(fd, LCD_IOCTL_WRITE, &display);
    display.Count = sprintf((char*)display.Msg, "===============");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    for(p = 0; p < 10; p++){
        num = p + 1;
        display.Count = sprintf((char*)display.Msg, "No.%2d: %d %s\n", num, rank[p].rank, rank[p].name);
        ioctl(fd, LCD_IOCTL_WRITE, &display);
    }
    display.Count = sprintf((char*)display.Msg, "===============");
    ioctl(fd, LCD_IOCTL_WRITE, &display);
    display.Count = sprintf((char*)display.Msg, "No.11: %d %s\n", rank[10].rank, rank[10].name);
    ioctl(fd, LCD_IOCTL_WRITE, &display);
    ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
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

    display.Count = sprintf((char*)display.Msg, "time: %d:%d:%d", hour, minute, second2p);
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
    // second = (tTime*tick_timer)/1000;
    
    if(second2p >= NUM)
    {
        // tTime = 0;
        minute = second2p/60;
        second2p = second2p % 60;
    }
    if(minute >= NUM)
    {
        // tTime = 0;
        hour = minute/60;
        minute = minute % 60;
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
    tmp_snake = head;
    tmp2_snake = head2p;

    display.CursorX = food.cy;
    display.CursorY = food.cx;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);

    display.Count = sprintf((char*)display.Msg, "$");
    ioctl(fd, LCD_IOCTL_WRITE, &display);

    if((COL-1==head2p->next->cy && 1==dir2p.cy)
        || (0==head2p->next->cy && -1==dir2p.cy)
        || (ROW-1==head2p->next->cx && 1==dir2p.cx)
        || (3==head2p->next->cx && -1==dir2p.cx))
    {
        over2p(1);
        // return;
    }

    tmp_snake = head;
    tmp2_snake = head2p;
    for(i = 0; i < length2p; i++){
        if( ((head2p->next->cy+dir2p.cy) == (tmp2_snake->next->cy)) && ((head2p->next->cx+dir2p.cx) == (tmp2_snake->next->cx)) ){
            over2p(2);
            // return;
        }
        else{
            tmp2_snake = tmp2_snake->next;
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

    // printf("1p pos x : %d y : %d\n", head->next->cx, head->next->cy);
    // printf("2p pos x : %d y : %d\n", head2p->next->cx, head2p->next->cy);

    
    if(who_eat == 1)
    {
        pthread_mutex_lock(&mutex);
        who_eat = 0;
        pthread_mutex_unlock(&mutex);

        lenChange = 1;
        length++;
        if(length >= 50)
        {
            over2p(3);
            // return;
        }

        // food.cy = rand() % COL;
        // food.cx = rand() % (ROW-3) + 3;
        change_ticker = 0;
        printf("1p eat\n");
        sem_wait(&food_sem);

        // sprintf(send_msg, "food %d %d", food.cx, food.cy);
        // if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
        //             errexit("Error : write()\n");
    }

    if(who_eat == 2)
    {
        pthread_mutex_lock(&mutex);
        who_eat = 0;
        pthread_mutex_unlock(&mutex);

        lenChange2p = 1;
        length2p++;
        if(length2p >= 50)
        {
            over2p(3);
            // return;
        }

        // food.cy = rand() % COL;
        // food.cx = rand() % (ROW-3) + 3;
        change_ticker2p = 0;
        printf("2p eat\n");
        sem_wait(&food_sem);
        // sprintf(send_msg, "food %d %d", food.cx, food.cy);
        // if(write(client_fd, send_msg, sizeof(send_msg)) == -1)
        //             errexit("Error : write()\n");
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
    // tmp_snake = head;
    // tmp2_snake = head2p;
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

void *show2p()
{

    while(game_over == 0){
        sem_wait(&refresh_sem);
        ioctl(fd, LCD_IOCTL_CLEAR, NULL);

        showSnake2p();
        showInformation2p();

        if(overMode != 0){
            // setTicker(0);
            over2p(overMode);
            
        }
    }
    pthread_exit(NULL);
}
 
void getOrder()
{
    int ret;
    unsigned short cmd;
    
    while(game_over == 0)
    {
        ret = ioctl(fd, KEY_IOCTL_WAIT_CHAR, &cmd);
        cmd = cmd & 0xff;

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
            dir2p.cx = 0;
            dir2p.cy = -1;

            sprintf(send_msg, "LEFT");
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 50)
        {
            dir2p.cx = -1;
            dir2p.cy = 0;

            sprintf(send_msg, "UP");
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 54)
        {
            dir2p.cx = 0;
            dir2p.cy = 1;

            sprintf(send_msg, "RIGHT");
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                        errexit("Error : write()\n");
        }
        else if(cmd == 56)
        {
            dir2p.cx = 1;
            dir2p.cy = 0;

            sprintf(send_msg, "DOWN");
            if(write(connfd, send_msg, sizeof(send_msg)) == -1)
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
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(2 == i){
        display.Count = sprintf((char*)display.Msg, "Crash itself.\nGame over");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
        // ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    }
    else if(3 == i){
        display.Count = sprintf((char*)display.Msg, "Mission Complete");
        ioctl(fd, LCD_IOCTL_WRITE, &display);
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

    sprintf(send_msg, "GameOver %d", i);
    if(write(connfd, send_msg, sizeof(send_msg)) == -1)
                errexit("Error : write()\n");

    // setTicker(0);             
    // deleteLink();            
    // exit(0);
    game_over = 1;

    display.CursorX = 0;
    display.CursorY = 12;
    ioctl(fd, LCD_IOCTL_CUR_SET, &display);
    display.Count = sprintf((char*)display.Msg, "Press any key \nto continue");
    ioctl(fd, LCD_IOCTL_WRITE, &display);
    ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
    new_round = 1;
    second2p = 0;

    
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
