//This is our server CUP. He has the duty to "intermediate"
//the User and the department for the managing of the visit.

#include "Cup_header.h"

int main(void) {
    
    //declaring variables
    char ipbuffer[INET_ADDRSTRLEN];
    pthread_t manager_tid;

    //Declaring list to null
    dep_list = NULL;

    //declaring struct variables 
    connectioninfo info;
    serverinfo s_info;
    clientinfo c_info;
    //department initially empty. 
    struct hostent* host;
    
    //cleaning the buffer
    memset(ipbuffer, 0, INET_ADDRSTRLEN);

    //instaling signal handling
    signal(EPIPE, SIG_IGN);

    //fetching date from the config file
    serverConfig(&info.__port, info.__ip);

    //setting up the socket
    s_info.__serverfd = secure_socket(AF_INET, SOCK_STREAM, 0);

    //signal(SIGINT, signal_cleaning);

    //setting the server address attributes
    s_info.__serveraddr.sin_family = AF_INET;
    s_info.__serveraddr.sin_port = htons(info.__port);
    s_info.__serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_info.__serverlen = sizeof(s_info.__serveraddr);


    //should add a sigaction to handle signal: TODO

    //binding the socket
    secure_bind(s_info.__serverfd, (struct sockaddr*)&s_info.__serveraddr, s_info.__serverlen);

    //going into listen 
    secure_listen(s_info.__serverfd, BACKLOG);

    //defining a size for the client addr structure
    c_info.__clientlen = sizeof(c_info.__clientaddr);

    //initialize the department from the config file and assign a thread to 
    //every department
    departmentConfig(&dep_list);

    //starting the server: Accepting connection
    system("clear");
    puts("CUP Server started!\n");
    puts("Waiting for incoming connection..\n");
    while(ONLINE) {
        //accept
        c_info.__clientfd = secure_accept(s_info.__serverfd, (struct sockaddr*)&c_info.__clientaddr, &c_info.__clientlen);
        puts("New client connected.");

        //know more about our new client
        secure_ntop(AF_INET, &(c_info.__clientaddr.sin_addr), ipbuffer, INET_ADDRSTRLEN);

        //getting host infos
        host = secure_gethostbyaddr(&(c_info.__clientaddr.sin_addr), INET_ADDRSTRLEN, AF_INET);

        puts("Infos:");
        printf("-IP: %s\n-Host name: %s\n-Host Port: %d\n", ipbuffer, host->h_name, ntohs(c_info.__clientaddr.sin_port));
        puts("");

        //now we need to greet the connected client and 
        //understand who's just connected: if a client or department.
        //create a thread manager to do that:
        secure_pthread_create(&manager_tid, NULL, thread_client, &c_info.__clientfd);

    }

}
