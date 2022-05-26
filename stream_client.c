#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>     //  Chứa cấu trúc cần thiết cho socket. 
#include <netinet/in.h>     //  Thư viện chứa các hằng số, cấu trúc khi sử dụng địa chỉ trên internet
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>


#define BUFF_SIZE 256
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

pthread_t tmp_thread, tmp_thread1;

void func(int signum)
{
    printf("\n!!!Type exit to escape safety!!!\n");
}
/* Chức năng chat*/
void *ptr_write_func(void *ptr_server_fd)
{   
    int server_fd = *(int *)ptr_server_fd;
    int numb_write, numb_read;
    char recvbuff[BUFF_SIZE];
    char sendbuff[BUFF_SIZE];
    tmp_thread1 = pthread_self();
    while (1) {
        memset(sendbuff, '0', BUFF_SIZE);
	    memset(recvbuff, '0', BUFF_SIZE);
        printf("\n>> ");
        fgets(sendbuff, BUFF_SIZE, stdin);   //fgets() lay data tu stdin va ghi vao sendbuff

        /* Gửi thông điệp tới server bằng hàm write 
        ** Write is blocking until data the content of buffer copied to kernel space
        ** write() ghi data tu sendbuff vao kernel space */
        numb_write = write(server_fd, sendbuff, sizeof(sendbuff));
        if (numb_write == -1)     
            handle_error("write()");
        if (strncmp("exit", sendbuff, 4) == 0) {
            printf("Client exit ...\n");
            pthread_cancel(tmp_thread);
            break;
        }
  
    }
    close(server_fd); /*close*/    
    
    pthread_exit(NULL); // exit
}


/* Chức năng chat */
void *ptr_read_func(void *ptr_server_fd)
{   
    int server_fd = *(int *)ptr_server_fd;   
    int numb_read, numb_write;
    char sendbuff[BUFF_SIZE];
    char recvbuff[BUFF_SIZE];
    tmp_thread = pthread_self();
    sleep(1);
    while (1) {        
        memset(sendbuff, '0', BUFF_SIZE);
        memset(recvbuff, '0', BUFF_SIZE);

        /* Đọc dữ liệu từ socket */
        /* Hàm read sẽ block cho đến khi đọc được dữ liệu */
        numb_read = read(server_fd, recvbuff, BUFF_SIZE);
        if (numb_read == -1) 
            handle_error("read()");   
        if (strncmp("exit", recvbuff, 4) == 0) {
            system("clear");
            pthread_cancel(tmp_thread1);
            break;
        }
        printf("\nServer: %s\n", recvbuff);
    }
    close(server_fd);
    pthread_exit(NULL);
}



int main(int argc, char *argv[])
{
    int portno;
    int server_fd;
    struct sockaddr_in serv_addr;
    pthread_t t_write, t_read;
    signal(SIGINT, func);
    //pthread_t t0;


	memset(&serv_addr, '0',sizeof(serv_addr));

    /* Đọc portnumber từ command line */
    if (argc < 3) {
        printf("command : ./client <server address> <port number>\n");
        exit(1);
    }
    portno = atoi(argv[2]);
	
    /* Khởi tạo địa chỉ server */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(portno);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) == -1) 
        handle_error("inet_pton()");
	
    /* Tạo socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        handle_error("socket()");
	
    /* Kết nối tới server*/
    if (connect(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        handle_error("connect()");

    printf("Client : got connection\n");
    printf("Start new chatting here...\n");

    /*Tao thread only write*/
    if (pthread_create(&t_write, NULL, ptr_write_func, &server_fd) != 0)
        handle_error("thread_write");
    
    /*Tao thread only read*/
    if (pthread_create(&t_read, NULL, ptr_read_func, &server_fd) != 0)
        handle_error("thread_read");
    /* Xu li thread khi ket thuc 
    Main thread cung se bi block tai day*/
    if (pthread_join(t_write, NULL) == -1)
        handle_error("thread_write_join");
    
    if (pthread_join(t_read, NULL) == -1)
        handle_error("thread_read_join");
    return 0;
}    
