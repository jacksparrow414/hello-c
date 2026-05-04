// 要暴露共享的常量,需要在头文件里面定义
#define BUFFER_SIZE 512
// 自定义状态码，表示非阻塞I/O模式下暂无数据可读
#define READ_AGAIN 4

int make_socket();
int bind_server_socket_to_port_and_listen();
int bind_client_socket_to_port_and_connect();
int read_from_client(int client_socket);
int set_nonblocking(int socket);
int print_time();
