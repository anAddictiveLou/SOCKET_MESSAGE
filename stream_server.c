#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>     //  Chứa cấu trúc cần thiết cho socket. 
#include <netinet/in.h>     //  Thư viện chứa các hằng số, cấu trúc khi sử dụng địa chỉ trên internet
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define LISTEN_BACKLOG 50
#define BUFF_SIZE 256
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

pthread_t tmp_thread, tmp_thread1;

/* Chức năng chat */

void *ptr_read_func(void *ptr_new_socket_fd)
{   
    int new_socket_fd = *(int *)ptr_new_socket_fd;   
    int numb_read, numb_write;
    char sendbuff[BUFF_SIZE];
    char recvbuff[BUFF_SIZE];
    tmp_thread1 = pthread_self();
	
    while (1) {        
        memset(sendbuff, '0', BUFF_SIZE);
        memset(recvbuff, '0', BUFF_SIZE);


        /* Đọc dữ liệu từ socket */
        /* Hàm read sẽ block cho đến khi đọc được dữ liệu */
        numb_read = read(new_socket_fd, recvbuff, BUFF_SIZE);
        if(numb_read == -1)
            handle_error("read()");
        if (strncmp("exit", recvbuff, 4) == 0) {
            printf("Client exit ....\n");
            pthread_cancel(tmp_thread);
            system("clear");
            break;
        }
        printf("\nClient: %s\n", recvbuff);
    }
    close(new_socket_fd);
    pthread_exit(NULL);
}

/* Chức năng chat */

void *ptr_write_func(void *ptr_new_socket_fd)
{   
    int new_socket_fd = *(int *)ptr_new_socket_fd;
    int numb_write, numb_read;
    char recvbuff[BUFF_SIZE];
    char sendbuff[BUFF_SIZE];
    tmp_thread = pthread_self();

    while (1) {
        memset(sendbuff, '0', BUFF_SIZE);
	    memset(recvbuff, '0', BUFF_SIZE);
        printf("\n");
        fgets(sendbuff, BUFF_SIZE, stdin);   //fgets() lay data tu stdin va ghi vao sendbuff

        /* Gửi thông điệp tới server bằng hàm write 
        ** Write is blocking until data the content of buffer copied to kernel space
        ** write() ghi data tu sendbuff vao kernel space */
        numb_write = write(new_socket_fd, sendbuff, sizeof(sendbuff));
        if (numb_write == -1)     
            handle_error("write()");
        if (strncmp("exit", sendbuff, 4) == 0) {
            pthread_cancel(tmp_thread1);
            printf("Unconnect to client ...\n");
            break;
        }
    }
    close(new_socket_fd); /*close*/ 
    pthread_exit(NULL); // exit
}



int main(int argc, char *argv[])
{
    int port_no, len, opt;
    int server_fd, new_socket_fd;
    struct sockaddr_in serv_addr, client_addr;
    pthread_t t_write, t_read;
    /* Đọc portnumber trên command line */
    if (argc < 2) {
        printf("No port provided\ncommand: ./server <port number>\n");
        exit(EXIT_FAILURE);
    } else
        port_no = atoi(argv[1]);

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    
    /* Tạo socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        handle_error("socket()");
    // fprintf(stderr, "ERROR on socket() : %s\n", strerror(errno));

    /* Ngăn lỗi : “address already in use” */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        handle_error("setsockopt()");  

    /* Khởi tạo địa chỉ cho server */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_no);
    serv_addr.sin_addr.s_addr =  INADDR_ANY; //inet_addr("192.168.5.128"); //INADDR_ANY

    /* Gắn socket với địa chỉ server */
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        handle_error("bind()");

    /* Nghe tối đa 5 kết nối trong hàng đợi */
    if (listen(server_fd, LISTEN_BACKLOG) == -1)
        handle_error("listen()");

    /* Dùng để lấy thông tin client */
	len = sizeof(client_addr);

    while (1) {
        printf("Server is listening at port : %d \n....\n",port_no);
		new_socket_fd  = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t *)&len); 
		if (new_socket_fd == -1)
            handle_error("accept()");

		system("clear");
		
		//char temp[BUFF_SIZE];
		//inet_ntop(client_addr.sin_family, (struct sockaddr*)&client_addr, temp, sizeof(temp));
		printf("Server : got connection \n");
        printf("Start new chatting here...\n");

        /*Tao thread only read*/
        if (pthread_create(&t_read, NULL, ptr_read_func, &new_socket_fd) != 0)
            handle_error("thread_read");
        
         /*Tao thread only write*/
        if (pthread_create(&t_write, NULL, ptr_write_func, &new_socket_fd) != 0)
            handle_error("thread_write");
        
        /* Xu li thread khi ket thuc */
        if (pthread_join(t_write, NULL) == -1)
            handle_error("thread_write_join");
        
        if (pthread_join(t_read, NULL) == -1)
            handle_error("thread_read_join");
        }
    close(server_fd);
    return 0;
}