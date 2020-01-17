//Defining an header to simplify and reduce the workload and written code onto 
//the main application file.
//We're defining here secured error-checked networking function and more, to 
//make our code POSIX SAFE and standardized.

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <mysql/mysql.h>
#include <sys/sendfile.h>

//global define
#define BACKLOG 32
#define CLIENT 0
#define OFFLINE 0
#define SERVER 1
#define ONLINE 1
#define DEPARTMENT 2

//defines codes
#define TERM 40
#define FAIL 41
#define FOUND 42
#define NOTFOUND 43
#define SUCCESS 44
#define DISCONNECTED 45

//define sizes
#define ACKSIZE 2
#define MINBUFSIZE 8
#define MAXIDSIZE 16
#define MAXDEPSIZE 32
#define BUFSIZE 32
#define MAXBUFSIZE 64

//define colors
#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"


//Global Variables
struct c_info {
    struct sockaddr_in __clientaddr;
    int __clientfd;
    socklen_t __clientlen;
};

struct s_info {
    struct sockaddr_in __serveraddr;
    int __serverfd;
    socklen_t __serverlen;
};

struct con_info {
    int __port;
    char __ip[INET_ADDRSTRLEN];
};

struct v_info {
    char __prenotationcode[BUFSIZE];
    char __recept[BUFSIZE];
    char __pfname[BUFSIZE];
    char __plname[BUFSIZE];
    char __date[MINBUFSIZE];
    char __time[MINBUFSIZE];
};

struct db_info {
    char __dbuser[BUFSIZE];
    char __dbuspws[BUFSIZE];
    char __dbname[BUFSIZE];
};

struct sk_info {
    int* sock_array;
    int  size;
};

static char* ACKNOWLEDGE = "OK";

//typedefs
typedef struct s_info serverinfo;
typedef struct c_info clientinfo;
typedef struct con_info connectioninfo;
typedef struct d_list departmentlist;
typedef struct ack ack_info;
typedef struct d_info departmentinfo;
typedef struct v_info visitinfo;
typedef struct db_info databaseinfo;
typedef struct sk_info sockinfo;
typedef void(*s_handler)(int);

//declaring our list and d_info later because we need the serverinfo declaration before.
struct d_list {
    char* __name;
    int __status;
    serverinfo __serverinfo;
    struct d_list* __next;
};

struct d_info {
    char* __name;
    int __status;
    serverinfo __depinfo;
};

//typedef departmentlist
typedef struct d_list departmentlist;


//-------Prototypes-------
//Connection - Server
int secure_socket(int __domain, int __type, int __protocol);
void secure_bind(int __sfd, const struct sockaddr* __saddr, socklen_t __addrlen);
void secure_listen(int __sfd, int __backlog);
int secure_accept(int __sfd, struct sockaddr* __caddr, socklen_t* __addrlen);

//Connection - Client
void secure_connect(int __sfd, const struct sockaddr* __saddr, socklen_t __addrlen);
void secure_server_init(connectioninfo __coninfo, serverinfo* __servinfo);

//Translation 
void secure_pton(int __af, const char* __src, void* __dst);
void secure_ntop(int __af, const void* __src, char* __dst, socklen_t __addrlen);
struct hostent* secure_gethostbyaddr(const void* __addr, socklen_t __len, int __type);

//Message Passing
int full_secure_send(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo);
int full_secure_recv(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo);
int full_secure_recv_with_ack(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo);
int full_secure_send_with_ack(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo);
void find_errno(sockinfo* __sinfo, int __mode);

//File Config 
void serverConfig(int* __port, char* __ip);
void departmentConfig(departmentlist **__list);
char** visitConfig();

//Department List
void insert_department(departmentlist** __list, char* __name, int __status, serverinfo __serverinfo);
void update_department_status(departmentlist** __list, char* __name, int __newstatus); 
void send_department_list(departmentlist** __list, int __cfd, pthread_mutex_t* __mutex);
int recv_department_list(int __sfd);
int send_choice_to_server(int __sfd);
departmentinfo* get_specific_department(departmentlist* __list, int __depnumber, pthread_mutex_t* __mutex);
int get_list_size(departmentlist* __list);
int try_department(departmentinfo** __dinfo, departmentlist** __list, int __cfd, char* __type, pthread_mutex_t* __mutex);

//Pthread_safe
void secure_pthread_create(pthread_t* __tid, const pthread_attr_t* __attr, void* (*__start_rtn)(void*), void* __arg);

//User choice
departmentinfo* get_department(int __cfd, departmentlist** __list, pthread_mutex_t* __mutex);
int send_department(int __sfd, int __choice);

//Error check
void scan_int(char* __msg, int* __value);

