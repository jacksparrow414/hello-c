// 要暴露共享的常量,需要在头文件里面定义
#define BUFFER_SIZE 512

int make_socket();
int bind_server_socket_to_port_and_listen();
int bind_client_socket_to_port_and_connect();
int read_from_client(int client_socket);
