#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>

#define PORT 8080
#define MAX_EVENTS 100
#define SUB_REACTOR_COUNT 2     // IO 搬运工数量
#define WORKER_THREAD_COUNT 4   // 业务算力工人数量
#define MAX_CLIENTS 10000

// --- 1. 连接上下文 ---
typedef struct {
    int fd;
    int sub_reactor_id;
    char out_buffer[8192]; // 发送缓冲区
    int out_len;
    pthread_mutex_t lock;  // 保护 out_buffer
} Connection;

Connection *connections[MAX_CLIENTS];

// --- 2. 核心组件结构 ---
typedef struct {
    int id;
    int epoll_fd;
    int wakeup_fd;
    int fd_queue[1024];
    int queue_count;
    pthread_mutex_t lock;
    pthread_t thread_id;
} SubReactor;

SubReactor sub_reactors[SUB_REACTOR_COUNT];

typedef struct {
    int client_fd;
    char *data;
} Task;

Task worker_queue[2048];
int worker_task_count = 0;
pthread_mutex_t worker_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t worker_queue_cond = PTHREAD_COND_INITIALIZER;

// --- 辅助函数 ---
void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void update_epoll(int epoll_fd, int fd, int events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

// 清理连接资源
void close_connection(int fd) {
    Connection *conn = connections[fd];
    if (conn) {
        close(fd); // 自动从 epoll 移除
        pthread_mutex_destroy(&conn->lock);
        free(conn);
        connections[fd] = NULL;
        printf("客户端 FD %d 断开连接，资源已清理\n", fd);
    }
}

// --- 3. 业务线程池 (专职算力) ---
void *worker_thread_loop(void *arg) {
    while (1) {
        Task task;
        pthread_mutex_lock(&worker_queue_lock);
        while (worker_task_count == 0) {
            pthread_cond_wait(&worker_queue_cond, &worker_queue_lock);
        }
        task = worker_queue[--worker_task_count];
        pthread_mutex_unlock(&worker_queue_lock);

        // 模拟业务处理耗时 (0.1秒)
        usleep(100000); 

        Connection *conn = connections[task.client_fd];
        if (conn) {
            char response[2048];
            int len = snprintf(response, sizeof(response), "[Worker Processed] %s", task.data);

            pthread_mutex_lock(&conn->lock);
            // 将结果安全地拼接到发送缓冲区
            if (conn->out_len + len < sizeof(conn->out_buffer)) {
                memcpy(conn->out_buffer + conn->out_len, response, len);
                conn->out_len += len;
                
                // 唤醒对应的 Sub-Reactor 去发数据
                int sub_epoll = sub_reactors[conn->sub_reactor_id].epoll_fd;
                update_epoll(sub_epoll, task.client_fd, EPOLLIN | EPOLLOUT | EPOLLET);
            } else {
                printf("警告：FD %d 的发送缓冲区已满，丢弃数据\n", task.client_fd);
            }
            pthread_mutex_unlock(&conn->lock);
        }
        free(task.data); // 释放通过 strdup 申请的内存
    }
    return NULL;
}

// --- 4. 子 Reactor (专职 IO，严格执行 ET 模式) ---
void *sub_reactor_loop(void *arg) {
    SubReactor *reactor = (SubReactor *)arg;
    struct epoll_event events[MAX_EVENTS];
    char buffer[1024];

    while (1) {
        int n = epoll_wait(reactor->epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            // --- 任务 1：处理主线程扔过来的新连接 ---
            if (fd == reactor->wakeup_fd) {
                uint64_t val;
                read(reactor->wakeup_fd, &val, sizeof(val)); 

                pthread_mutex_lock(&reactor->lock);
                for (int j = 0; j < reactor->queue_count; j++) {
                    int new_client = reactor->fd_queue[j];
                    set_nonblocking(new_client);
                    connections[new_client]->sub_reactor_id = reactor->id;

                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLET; // 注册边缘触发
                    client_ev.data.fd = new_client;
                    epoll_ctl(reactor->epoll_fd, EPOLL_CTL_ADD, new_client, &client_ev);
                }
                reactor->queue_count = 0;
                pthread_mutex_unlock(&reactor->lock);
            } 
            // --- 任务 2：处理客户端 IO ---
            else {
                Connection *conn = connections[fd];
                if (!conn) continue;

                // [ET 修复] 可读事件：必须用 while 循环榨干内核缓冲区！
                if (events[i].events & EPOLLIN) {
                    while (1) {
                        ssize_t count = read(fd, buffer, sizeof(buffer) - 1);
                        if (count > 0) {
                            buffer[count] = '\0';
                            
                            // 读到一块数据，就扔给业务线程池
                            pthread_mutex_lock(&worker_queue_lock);
                            worker_queue[worker_task_count].client_fd = fd;
                            worker_queue[worker_task_count].data = strdup(buffer);
                            worker_task_count++;
                            pthread_cond_signal(&worker_queue_cond);
                            pthread_mutex_unlock(&worker_queue_lock);
                        } 
                        else if (count == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 数据终于读干了，可以安心退出了
                                break;
                            } else {
                                // 真正发生错误
                                close_connection(fd);
                                break;
                            }
                        } 
                        else if (count == 0) {
                            // 客户端主动断开
                            close_connection(fd);
                            break;
                        }
                    }
                }

                // [ET 修复] 可写事件：尽量发，直到发完或者遇到 EAGAIN
                if (conn && (events[i].events & EPOLLOUT)) {
                    pthread_mutex_lock(&conn->lock);
                    
                    while (conn->out_len > 0) {
                        ssize_t sent = send(fd, conn->out_buffer, conn->out_len, 0);
                        if (sent > 0) {
                            conn->out_len -= sent;
                            if (conn->out_len > 0) {
                                // 将没发完的数据往前挪
                                memmove(conn->out_buffer, conn->out_buffer + sent, conn->out_len);
                            }
                        } else if (sent == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 内核发送缓冲区满了，立刻停手，等下次 EPOLLOUT
                                break; 
                            } else {
                                perror("send error");
                                close_connection(fd);
                                break;
                            }
                        }
                    }

                    // 发完了，取消关注 EPOLLOUT，只保留 EPOLLIN
                    if (conn && conn->out_len == 0) {
                        update_epoll(reactor->epoll_fd, fd, EPOLLIN | EPOLLET);
                    }
                    
                    pthread_mutex_unlock(&conn->lock);
                }
            }
        }
    }
    return NULL;
}

// --- 5. 主线程 (专职 Accept) ---
int main() {
    // 1. 初始化资源
    for(int i=0; i<MAX_CLIENTS; i++) connections[i] = NULL;

    for(int i=0; i<WORKER_THREAD_COUNT; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, worker_thread_loop, NULL);
    }

    for (int i = 0; i < SUB_REACTOR_COUNT; i++) {
        sub_reactors[i].id = i;
        sub_reactors[i].queue_count = 0;
        pthread_mutex_init(&sub_reactors[i].lock, NULL);
        sub_reactors[i].epoll_fd = epoll_create1(0);
        sub_reactors[i].wakeup_fd = eventfd(0, EFD_NONBLOCK);
        
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = sub_reactors[i].wakeup_fd;
        epoll_ctl(sub_reactors[i].epoll_fd, EPOLL_CTL_ADD, sub_reactors[i].wakeup_fd, &ev);
        
        pthread_create(&sub_reactors[i].thread_id, NULL, sub_reactor_loop, &sub_reactors[i]);
    }

    // 2. 创建 Server Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, SOMAXCONN);
    set_nonblocking(server_fd); // 必须非阻塞

    // 3. 主 Reactor
    int main_epoll_fd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;  // 主 Reactor 也要用 ET
    event.data.fd = server_fd;
    epoll_ctl(main_epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    printf(">>> NIO 终极架构启动 (严格 ET 模式) <<<\n");
    printf("PORT: %d | Main(1) + Sub(%d) + Worker(%d)\n", PORT, SUB_REACTOR_COUNT, WORKER_THREAD_COUNT);

    int next_reactor = 0;
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(main_epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                int client_fd;
                // [ET 修复] Accept 必须榨干队列！
                while ((client_fd = accept(server_fd, NULL, NULL)) > 0) {
                    if (client_fd >= MAX_CLIENTS) { 
                        printf("达到最大连接数，拒绝连接\n");
                        close(client_fd); 
                        continue; 
                    }

                    Connection *conn = malloc(sizeof(Connection));
                    conn->fd = client_fd;
                    conn->out_len = 0;
                    pthread_mutex_init(&conn->lock, NULL);
                    connections[client_fd] = conn;

                    int target = next_reactor % SUB_REACTOR_COUNT;
                    next_reactor++;
                    
                    SubReactor *reactor = &sub_reactors[target];
                    pthread_mutex_lock(&reactor->lock);
                    reactor->fd_queue[reactor->queue_count++] = client_fd;
                    uint64_t one = 1;
                    write(reactor->wakeup_fd, &one, sizeof(one)); 
                    pthread_mutex_unlock(&reactor->lock);
                }
                
                // 处理 accept 的错误
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept error");
                    }
                }
            }
        }
    }
    return 0;
}
