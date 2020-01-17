//Extern file to store function and procedures of the secure networking header.

#include "secure_networking.h"

//------Definitions-------

//------ Connections - Server ------
int secure_socket(int __domain, int __type, int __protocol) {
    int __retfd;
    if((__retfd = socket(__domain, __type, __protocol)) == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    return __retfd;
}

void secure_bind(int __sfd, const struct sockaddr* __saddr, socklen_t __addrlen) {
    if(bind(__sfd, __saddr, __addrlen) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

void secure_listen(int __sfd, int __backlog) {
    if(listen(__sfd, __backlog) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

int secure_accept(int __sfd, struct sockaddr* __caddr, socklen_t* __addrlen) {
    int __retfd;
    if((__retfd = accept(__sfd, __caddr, __addrlen)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
}

//------ Connection - Client ------
void secure_connect(int __sfd, const struct sockaddr* __saddr, socklen_t __addrlen) {
    if(connect(__sfd, __saddr, __addrlen) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

void secure_server_init(connectioninfo info, serverinfo* s_info) {
    //socket creation
    s_info->__serverfd = secure_socket(AF_INET, SOCK_STREAM, 0);

    //assigning properties to server info
    s_info->__serveraddr.sin_family = AF_INET;
    s_info->__serveraddr.sin_port = htons(info.__port);
    secure_pton(AF_INET, info.__ip, &s_info->__serveraddr.sin_addr.s_addr);

    //defining the server address lenght
    s_info->__serverlen = sizeof(s_info->__serveraddr);
}



//------ Secure translation ------
void secure_pton(int __af, const char* __src, void* __dst) {
    if(inet_pton(__af, __src, __dst) == -1) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }


}

void secure_ntop(int __af, const void* __src, char* __dst, socklen_t __addrlen) {
    if(inet_ntop(__af, __src, __dst, __addrlen) == NULL) {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }
}

struct hostent* secure_gethostbyaddr(const void* __addr, socklen_t __len, int __type) {
    if (gethostbyaddr(__addr, __len, __type) == NULL) {
        perror("gethostbyaddr");
        exit(EXIT_FAILURE);
    }
}


//------ Sending message ------
int full_secure_send(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo) {
    int retval, bytes_sent = 0;
    while (bytes_sent < __buflen) { //while still to send
        retval = send(__sfd, __buf, __buflen, __flags);
        //check for errors
        if (retval == -1) {
            //if something interrupted the read
            if(errno == EINTR) {
                //resume from the last byte received
                retval = 0;
            } else {
                find_errno(__sinfo, __mode);
            }
        } 
        else if (retval == 0) { //EOF reached or disconnecting
            find_errno(__sinfo, __mode);
        }

        //if here, data was sent
        //updating the buf position and the remaining buf lenght
        __buf += bytes_sent;
        __buflen -= bytes_sent;

        //increasing the number of bytes sent
        bytes_sent += retval;
    }

    return bytes_sent;

}

int full_secure_recv(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo) {
    int retval, bytes_received = 0;
    while(bytes_received < __buflen){ //while still to receive
        retval = recv(__sfd, __buf, __buflen, __flags);
        //check for errors
        if (retval == -1) {
            //if something interrupted the read
            if(errno == EINTR) {
                //resume from the last byte received
                retval = 0;
            } else {
                find_errno(__sinfo , __mode);
            }
        } 
        else if (retval == 0) { //EOF reached or disconnecting
            find_errno(__sinfo, __mode);
        }
        
        //if here, data was received
        //updating the buf position and the remaining buf lenght
        __buf += bytes_received;
        __buflen -= bytes_received;

        //increasing the number of bytes received
        bytes_received += retval;
        
    }

    return bytes_received;

}

int full_secure_recv_with_ack(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo) {
    //receiving data
    int bytes_recv = full_secure_recv(__sfd, __buf, __buflen, __flags, __mode, __sinfo);

    //telling the sender that the data was successful read
    full_secure_send(__sfd, ACKNOWLEDGE, ACKSIZE, MSG_NOSIGNAL, __mode, __sinfo);
    //return the byte recvd
    return bytes_recv;
}

int full_secure_send_with_ack(int __sfd, void* __buf, size_t __buflen, int __flags, int __mode, sockinfo* __sinfo) {
    //declaring ack buffer and cleaning it
    char ACKNOWLEDGE_RECV[ACKSIZE];
    memset(ACKNOWLEDGE_RECV, 0, ACKSIZE);
    
    //sending data
    int bytes_sent = full_secure_send(__sfd, __buf, __buflen, __flags, __mode, __sinfo);

    //waiting for the acknowedge of the data sent from the receiver
    full_secure_recv(__sfd, ACKNOWLEDGE_RECV, ACKSIZE, 0, __mode, __sinfo);
    //return byte sent
    return bytes_sent;
}

void find_errno(sockinfo* __sinfo, int __mode) {

    switch(errno) {    
        case ECONNRESET:
            //if client resetted con
            for(int i=0; i<__sinfo->size; i++) {
                close(__sinfo->sock_array[i]);
            }
            if(__mode == SERVER) {
                perror("Thread exiting:");
                pthread_exit(NULL);
            } 
            else {
                fprintf(stderr, "Server reset connection, exiting..\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        case ECONNREFUSED:
            //close con
            for(int i=0; i<__sinfo->size; i++) {
                close(__sinfo->sock_array[i]);
            }
            if(__mode == SERVER) {
                perror("Thread exiting:");
                pthread_exit(NULL);
            } else {
                fprintf(stderr, "Server refused connection, exiting..\n");
                exit(EXIT_FAILURE);
            }
            break;

        case EPIPE:
            //close con
            for(int i=0; i<__sinfo->size; i++) {
                close(__sinfo->sock_array[i]);
            }
            if(__mode == SERVER) {
                perror("Thread exiting:");
                pthread_exit(NULL);
            } 
            else {
                //if server close con, return
                fprintf(stderr, "Could not send/receive data to/from server, exiting..\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            //close con
            for(int i=0; i<__sinfo->size; i++) {
                close(__sinfo->sock_array[i]);
            }
            if(__mode == SERVER) {
                perror("Thread exiting:");
                pthread_exit(NULL);
            }
            else {
                fprintf(stderr, "Could not reach the server, exiting.\n");
                exit(EXIT_FAILURE);
            }
        break;

    }

}


//------ File Configuration ------
void serverConfig(int* __port, char* __ip) {

    //variables
    FILE* configfile;
    char c;
    char buffer[INET_ADDRSTRLEN];
    int count = 0;

    //open the file 
    //check why ./ConfigFile/filename doesnt work.
    if((configfile = fopen("./ConfigFile/server_config.txt", "r")) == NULL) {
        fprintf(stderr, "Could not locate the File.\n");
        fprintf(stderr, "Assure that the folder ConfigFile and the server_config.txt exists.\n");
        fprintf(stderr, "server_config contents: <PORT:IP>\n");
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //fetching PORT:IP from file 
    while((c = fgetc(configfile)) != EOF) {
        if(c != ':' && c != '\n') {
            buffer[count] = c;
            count++;
        } else if(c == ':') {
            *__port = atoi(buffer);
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        } else {
            break; //newline reached: we only need to analyze the first line.
        }
    }

    //clean the input buffer
    memset(__ip, 0, INET_ADDRSTRLEN);

    //coping the content
    strcat(__ip, buffer);

    //close the file
    fclose(configfile);
}

void departmentConfig(departmentlist** __list) {

    //declaring variables
    FILE* configfile;
    char c;
    char buffer[MAXBUFSIZE];
    char IP[INET_ADDRSTRLEN];
    int port;
    int count = 0;

    //socket info
    sockinfo socket_info;

    //connection details
    serverinfo dep_info;
    connectioninfo info;

    if((configfile = fopen("./ConfigFile/department_config.txt", "r")) == NULL) {
        fprintf(stderr, "Could not locate the File.\n");
        fprintf(stderr, "Assure that the folder ConfigFile and the department_config.txt exists.\n");
        fprintf(stderr, "department_config contents: <PORT:IP> on multiple lines.\n");
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //fetching PORT:IP-NAME from file 
    while((c = fgetc(configfile)) != EOF) {
        if(c != ':' && c != '\n' && c != '-') {
            buffer[count] = c;
            count++;
        } else if(c == ':') {
            port = atoi(buffer);
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        } else if (c == '-') {
            strcpy(IP, buffer);
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        }    
          else { //case when newline reached, try to connect
            //N.B: at this time, buffer contains the name of the department.
            //clear the struct 
            memset(&dep_info, 0, sizeof(dep_info));
            memset(&info, 0, sizeof(info));

            //info setup
            strcpy(info.__ip, IP);
            info.__port = port;

            //secure server init 
            secure_server_init(info, &dep_info);

            //connect to the department:
            if(connect(dep_info.__serverfd, (struct sockaddr*)&dep_info.__serveraddr, dep_info.__serverlen) == -1) {
                //if connect return -1 (could not connect) insert into the list the server with offline status.
                insert_department(__list, buffer, OFFLINE, dep_info);
            } else { //department online
                insert_department(__list, buffer, ONLINE, dep_info);
                //send the ID to the department 
                char* ID = "TRYCON";
                int stringlen = strlen(ID);
                socket_info.sock_array = &dep_info.__serverfd;
                full_secure_send_with_ack(dep_info.__serverfd, &stringlen, sizeof(int), MSG_NOSIGNAL, CLIENT, &socket_info);
                full_secure_send_with_ack(dep_info.__serverfd, ID, stringlen, 0, CLIENT, &socket_info);
            }

            //close the fd of the department:
            //we just wanted to check the connection
            close(dep_info.__serverfd);

            //clean the buffer and reset the count
            count = 0;
            memset(buffer, 0, sizeof(buffer));

        }
    }
}

char** visitConfig() {

    //variables
    FILE* configfile;
    char c;
    char buffer[MINBUFSIZE];
    char** visitlist;
    int bcount = 0;
    int lcount = 0;

    //init 
    visitlist = malloc(16*sizeof(char*));
    for(int i=0; i<16; i++) {
        visitlist[i] = malloc(MINBUFSIZE*sizeof(char));
    }

    //cleaning
    memset(buffer, 0, MINBUFSIZE);

    //open the file 
    //check why ./ConfigFile/filename doesnt work.
    if((configfile = fopen("./ConfigVisit/visit_config.txt", "r")) == NULL) {
        fprintf(stderr, "Could not locate the File.\n");
        fprintf(stderr, "Assure that the folder ConfigFile and the server_config.txt exists.\n");
        fprintf(stderr, "server_config contents: <PORT:IP>\n");
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //fetching PORT:IP from file 
    while((c = fgetc(configfile)) != EOF) {
        if(c != '\n') {
            buffer[bcount] = c;
            bcount++;
        } else {
            //copying
            strcpy(visitlist[lcount], buffer);
            lcount++;

            //resetting
            memset(buffer, 0, MINBUFSIZE);
            bcount = 0;

        }
    }


    //close the file
    fclose(configfile);

    //return the array
    return visitlist;
}


//------ Department List ------
void insert_department(departmentlist** __list, char* __name, int __status, serverinfo __serverinfo) {
    
    departmentlist* newNode = malloc(sizeof(departmentlist));
    departmentlist* refNode = (*__list);

    //base case: list is empty 
    if((*__list) == NULL) {
        //updating the newnode
        newNode->__name = malloc(sizeof(strlen(__name)+1));
        strcpy(newNode->__name, __name);
        newNode->__status = __status;
        newNode->__serverinfo = __serverinfo;
        newNode->__next = NULL;

        //assigning the new head
        (*__list) = newNode;

    } else { //list not empty, find the first spot
        //using the ref node
        while(refNode->__next != NULL) {
            refNode = refNode->__next;
        }

        //once found the empty spot
        //init newnode
        newNode->__name = malloc(sizeof(strlen(__name)+1));
        strcpy(newNode->__name, __name);
        newNode->__status = __status;
        newNode->__serverinfo = __serverinfo;
        newNode->__next = NULL;

        //assign it to refnode
        refNode->__next = newNode;        
    }
}

void update_department_status(departmentlist** __list, char* __name, int __newstatus) {
    
    bool isFound;
    departmentlist* refNode = (*__list);

    //base case: list is empty
    if((*__list) == NULL) {
        printf("Department List is empty.\n");
        return;
    } else {
        //while list not empty
        while(refNode != NULL) {
            //if desired department found
            if(strcmp(refNode->__name, __name) == 0) {
                //update the status
                refNode->__status = __newstatus;
                //tell found
                isFound = true;
                //go break
                break;
            }

            //else just move on
            refNode = refNode->__next;

        }

        if(!isFound) {
            printf("Department not found into the list.\n");
            return;
        } 
    
        return;

    }
}

void send_department_list(departmentlist** __list, int __cfd, pthread_mutex_t* __mutex) {
    
    //declaring buffer
    char buffer[MAXBUFSIZE];
    char EOC[MINBUFSIZE];
    int stringlen;
    int check = 1;
    int status;

    //socket info
    sockinfo socket_info;

    //updating socket info
    socket_info.sock_array = &__cfd;
    socket_info.size = 1;

    //using a dummy reference to the list to iterate the list itself
    departmentlist* refList = (*__list);

    //delcaring a dep info structure to pass to try department
    departmentinfo* dep_info = malloc(sizeof(departmentinfo));

    //cleaning buffer
    memset(EOC, 0, MINBUFSIZE);

    //base case: list empty
    if(refList == NULL) {
        sprintf(buffer, "NULL");
        stringlen = strlen(buffer);

        //sendint buffer size and buffer to the client
        full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        full_secure_send_with_ack(__cfd, buffer, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);

    } else { //list no null
        //while list not null and client not disconnected
        while(refList != NULL && strcmp(buffer, "EOC") != 0) {

            //clean buffer for new entry
            memset(buffer, 0, MAXBUFSIZE);

            //try current department:
            //build the dep info struct
            dep_info->__name = malloc(strlen(refList->__name)+1);
            strcpy(dep_info->__name, refList->__name);

            //need to check the status in mutual exclusion: it could be updated
            //at the time we check it.
            pthread_mutex_lock(__mutex);
            dep_info->__status = refList->__status;
            pthread_mutex_unlock(__mutex);

            dep_info->__depinfo.__serveraddr = refList->__serverinfo.__serveraddr;
            dep_info->__depinfo.__serverlen = sizeof(dep_info->__depinfo.__serveraddr);
        
            //try the department
            status = try_department(&dep_info, __list, 0, "TRYCON", __mutex);
            

            //copy buffer content
            sprintf(
                buffer, 
                "\x1b[34m%-25s\x1b[0m%-25s\n", 
                refList->__name, 
                (refList->__status != 0?
                "\033[32mONLINE\033[0m": 
                "\033[31mOFFLINE\033[0m")
                );
            stringlen = strlen(buffer);
            
            //sending buffer size and buffer to the client
            full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
            full_secure_send_with_ack(__cfd, buffer, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);
            
            //update list
            refList = refList->__next;
            
        }

        //if here, maybe client disconnected? check it
        if(strcmp(buffer, "EOC") == 0) {
            printf("Client disconnected on send department list.\n");
        } else { //otherwise, tell him that no more department are up
            memset(EOC, 0, MINBUFSIZE);
            strcpy(EOC, "EOC");
            stringlen = strlen(EOC);
            //send EOC size and EOC message to client
            full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
            full_secure_send_with_ack(__cfd, EOC, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);
        }
    }

}

int recv_department_list(int __sfd) {
    //declaring variable
    int count = 1;
    int stringlen;
    char generalbuffer[MAXBUFSIZE];

    //cleaning the buffer
    memset(generalbuffer, 0, MAXBUFSIZE);

    //socket info
    sockinfo socket_info;
    socket_info.sock_array = &__sfd;
    socket_info.size = 1;

    puts("");
    puts("Available Departments:\n");
    printf("%-28s%-28s\n","Department", "Status");
    //receive each department size before receiving the department name
    while(full_secure_recv_with_ack(__sfd, &stringlen, sizeof(int), 0, CLIENT, &socket_info) && strcmp(generalbuffer, "EOC") != 0) {

        //receive the department name with the specified size
        full_secure_recv_with_ack(__sfd, generalbuffer, stringlen, 0, CLIENT, &socket_info);

        //check the list not null
        if(strcmp(generalbuffer, "NULL") == 0) {
            printf("No available department found. Try later!\n");
            puts("");
            //return failure
            return TERM;
        } 
        else if(strcmp(generalbuffer, "EOC") == 0) { //else the list is finished
            break; //if finished break;
        }

        //print info and clean
        printf("%d. %s", count, generalbuffer);
        memset(generalbuffer, 0, MAXBUFSIZE);
        count++;

    } puts("");

    //anythin good? return 1
    return 1;
}

int send_choice_to_server(int __sfd) {

    //declaring variables
    int depnum;

    //socket info
    sockinfo socket_info;
    socket_info.sock_array = &__sfd;
    socket_info.size = 1;

    scan_int("Choose your department (DEP. NUM or -1 to go back)", &depnum);    

    //check if the user want to go back
    if(depnum == -1) {
        //send the abort to the server
        depnum = TERM;
        full_secure_send_with_ack(__sfd, &depnum, sizeof(int), 0, CLIENT, &socket_info);
        return depnum;
    }

    //reset the stdin from \n
    getchar();

    //send the request to the server
    full_secure_send_with_ack(__sfd, &depnum, sizeof(int), 0, CLIENT, &socket_info);

    //success
    return 1;

}

departmentinfo* get_specific_department(departmentlist* __list, int __depnumber, pthread_mutex_t* __mutex) {

    //we need to search the depnumber department and return it's file descriptor.
    //until we reached the desired node
    departmentinfo* dep_info = malloc(sizeof(departmentinfo));

    //check if the request is valid:
    int safe;
    if((safe = get_list_size(__list)) != 0) { //if list not null
        if (__depnumber > safe) { //if reqeusted number > list size
            return NULL; //error: requested non-existent department.
        }
    }

    for(int i=1; i<__depnumber; i++) {
        //advance in the list
        __list = __list->__next;
    }

    //return the dep info
    //alloc memory for the name
    dep_info->__name = malloc(strlen(__list->__name)+1); //+1 is the '\0'

    //copy content
    strcpy(dep_info->__name, __list->__name);
    dep_info->__depinfo.__serveraddr = __list->__serverinfo.__serveraddr;

    //copy the status in mutual exclusion
    pthread_mutex_lock(__mutex);
    dep_info->__status = __list->__status;
    pthread_mutex_unlock(__mutex);
    
    dep_info->__depinfo.__serverlen = sizeof(dep_info->__depinfo.__serveraddr);


    return dep_info;

}

int get_list_size(departmentlist* __list) {
    if(__list == NULL) {
        return 0;
    } else {
        int counter = 0;
        while(__list != NULL) {
            __list = __list->__next;
            counter++;
        }

        return counter;
    }
}

int try_department(departmentinfo** __dinfo, departmentlist** __list, int __cfd, char* __type, pthread_mutex_t* __mutex) {

    //declaring variables
    int status;
    int stringlen;
    char* ID = __type;

    //socket info
    sockinfo socket_info;

    //generating structure
    socket_info.sock_array = &__cfd;
    socket_info.size = 1;

    //create a new socket for the department
    (*__dinfo)->__depinfo.__serverfd = secure_socket(AF_INET, SOCK_STREAM, 0);

    //connect to the specified department to check if still online
    if((status = connect((*__dinfo)->__depinfo.__serverfd, 
        (struct sockaddr*)&(*__dinfo)->__depinfo.__serveraddr, 
        (*__dinfo)->__depinfo.__serverlen)) == -1) {
            //if not online, update its status with OFFLINE
            //if the status is different from ONLINE
            pthread_mutex_lock(__mutex);
            if((*__dinfo)->__status != OFFLINE) {
                //put the server offline
                update_department_status(__list, (*__dinfo)->__name, OFFLINE);
                pthread_mutex_unlock(__mutex);
            } else {
                pthread_mutex_unlock(__mutex);
            }

            //send 0 to the client to let him know that the department went offline.
            
            //cfd could be zero if try department is called from send department list:
            //we dont need to inform the client.
            if(__cfd != 0) {
                status = 0;
                full_secure_send_with_ack(__cfd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
            }

            //closing
            close((*__dinfo)->__depinfo.__serverfd);

            //return -1 to notify the failure.
            return -1;
    } else {
        //if the status is different from online
        pthread_mutex_lock(__mutex);
        if((*__dinfo)->__status != ONLINE) {
            //update status
            update_department_status(__list, (*__dinfo)->__name, ONLINE);
            pthread_mutex_unlock(__mutex);
        } else {
            pthread_mutex_unlock(__mutex);
        }
            
        //send identifier and its size
        int stringlen = strlen(ID);
        full_secure_send_with_ack((*__dinfo)->__depinfo.__serverfd, &stringlen, sizeof(int), 0, CLIENT, &socket_info);
        full_secure_send_with_ack((*__dinfo)->__depinfo.__serverfd, ID, stringlen, 0, CLIENT, &socket_info);

        //receive happened connection status
        //if __type is CUP, we care to receive something, else do nothing and return.            
        if(strcmp(__type, "CUP") == 0) {    
            //inform the client it was found
            status = 1;
            full_secure_send_with_ack(__cfd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

            //receiving greeting ack from department (1 if is ok 0 if not)
            full_secure_recv_with_ack((*__dinfo)->__depinfo.__serverfd, &status, sizeof(int), 0, CLIENT, &socket_info);
        }
    }


}

//------ Pthread safe functions ------ 
void secure_pthread_create(pthread_t* __tid, const pthread_attr_t* __attr, void* (*__start_rtn)(void*), void* __arg) {
    int errnum;
    if((errnum = pthread_create(__tid, __attr, __start_rtn, __arg)) != 0) {
        fprintf(stderr, "pthread_create: %d", errnum);
        exit(EXIT_FAILURE);
    }
}

//User choice
departmentinfo* get_department(int __cfd, departmentlist** __list, pthread_mutex_t* __mutex) {

//declaring variables
    char response[MINBUFSIZE];
    char ack[ACKSIZE];
    int status;
    int depchoice;

    //socket info
    sockinfo socket_info;

    //generating structure
    socket_info.sock_array = &__cfd;
    socket_info.size = 1;

    //declaring departmentinfo for the department
    departmentinfo* dep_info = malloc(sizeof(departmentinfo));

    //cleaning the buffer
    memset(ack, 0, ACKSIZE);
    memset(response, 0, MINBUFSIZE);

    //first thing: send to the client the department list.
    send_department_list(__list, __cfd, __mutex);

    //if the list is null, useless going further
    if((*__list) == NULL) {
        char* depresponse = "NULL";
        int stringlen = strlen(depresponse);
        //sending depresponse size and buffer
        full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        full_secure_send_with_ack(__cfd, depresponse, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);
        return NULL; //nothing more to do here, go back
    }

    //receive the choosed department  
    full_secure_recv_with_ack(__cfd, &depchoice, sizeof(int), 0, SERVER, &socket_info);

    //if the client went back, return NULL
    if(depchoice == TERM) {
        return NULL;
    }

    //searching the corrispondent server into the list
    dep_info = get_specific_department((*__list), depchoice, __mutex);

    //notify the client about his request
    if(dep_info == NULL) { //not found, notify the client
        char* depresponse = "NOTFOUND";
        int stringlen = strlen(depresponse);
        //sending depresponse size and buffer
        full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        full_secure_send_with_ack(__cfd, depresponse, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);
        return NULL;
    } else {
        char* depresponse = "FOUND";
        int stringlen = strlen(depresponse);
        //sending depresponse size and buffer
        full_secure_send_with_ack(__cfd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        full_secure_send_with_ack(__cfd, depresponse, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);
    }

    //anything went well, return the found department info
    return dep_info;

}

int send_department(int __sfd, int __choice) {

    //declaring variables
    char response[MINBUFSIZE];
    int status;
    int stringlen;

    //socket info
    sockinfo socket_info;
    socket_info.sock_array = &__sfd;
    socket_info.size = 1;

    //cleaning the buffer
    memset(response, 0, MINBUFSIZE);

    //send the server the choice we made (1,2,3 etc)
    full_secure_send_with_ack(__sfd, &__choice, sizeof(int), 0, CLIENT, &socket_info);

    //recv the dep list from the server
    if((status = recv_department_list(__sfd)) == TERM) {
        status = -1;
        return status;
    }

    //send the choosed department to the server:
    //if the answer is -1, return back
    if((status = send_choice_to_server(__sfd)) == TERM) {
        status = -1;
        return status;
    }

    //get response from server to check if the department number choosed was good
    //getting size of response and response buffer
    full_secure_recv_with_ack(__sfd, &stringlen, sizeof(int), 0, CLIENT, &socket_info);
    full_secure_recv_with_ack(__sfd, response, stringlen, 0, CLIENT, &socket_info);

    //get response from server to check if the department number choosed was good
    if(strcmp(response, "NOTFOUND") == 0) {
        printf("Department not recognized. Make sure you entered the right number.\n");
        puts("");
        //return failure
        status = -1;
    } 
    //if the list is null
    else if(strcmp(response, "NULL") == 0) {
        printf("No current active server. Try later.\n");
        puts("");
        //return failure
        status = -1;
    }
    //else the department was found, but we must check
    //if it is online.
    else {
        //receive the answer from server
        full_secure_recv_with_ack(__sfd, &status, sizeof(int), 0, CLIENT, &socket_info);
    }

    //anything good, return success
    return status;
}

//------ Error Check ------
void scan_int(char* __msg, int* __var) {

    //declaring variables
    bool correct = false;
    int c;

    printf("%s: ", __msg);
	    while(!correct) {
		    if(scanf("%d", __var) != 1) {
			    printf("Value not correct, please try again.\n");
			    printf("%s: ", __msg);
			    while((c = getchar()) != '\n' && c != EOF);
		    } else {
			    correct = true;
		    }	
    	}
}
