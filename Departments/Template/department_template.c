//Medical department for Surgery

#include "Department_header.h"

int main(void) {

    //declaring variables 
    pthread_t manager_tid;
    char* department_name = "INSERT_NAME_HERE";
    char* database_username = "INSERT_USERNAME_HERE";
    char* database_password = "INSERT_PASSWORD_HERE";
    char* database_name = department_name;
    char** visitlist = visitConfig();

    //handling signal
    signal(EPIPE, SIG_IGN);
   
    //declaring struct variables
    connectioninfo info;
    serverinfo s_info;
    clientinfo c_info;
    databaseinfo db_info;

    //populating the db_info struct
    strcpy(db_info.__dbuser, database_username);
    strcpy(db_info.__dbuspws, database_password);
    strcpy(db_info.__dbname, database_name);

    //setting global info list
    globaldb_info = db_info;
    globallist = (char**)malloc((sizeof(visitlist))*sizeof(char*));
    for(int i=0; i<=sizeof(visitlist); i++) {
        globallist[i] = malloc(strlen(visitlist[i])+1);
        memcpy(globallist[i], visitlist[i], strlen(visitlist[i])+1);
    } 

    //config the server info
    serverConfig(&info.__port, info.__ip);

    //creating the socket
    s_info.__serverfd = secure_socket(AF_INET, SOCK_STREAM, 0);

    //setting the attributes
    s_info.__serveraddr.sin_family = AF_INET;
    s_info.__serveraddr.sin_port = htons(info.__port);
    s_info.__serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_info.__serverlen = sizeof(s_info.__serveraddr);

    //binding the socket
    secure_bind(s_info.__serverfd, (struct sockaddr*)&s_info.__serveraddr, s_info.__serverlen);

    //putting the socket on listen
    secure_listen(s_info.__serverfd, BACKLOG);

    //defining a size for the client addr structure
    c_info.__clientlen = sizeof(c_info.__clientaddr);

    //starting the server: Accepting connection
    system("clear");
    printf("Department %s ONLINE!\n", department_name);
    puts("Waiting for incoming connection..\n");

    //starting accepting the connections
    while(ONLINE) {

        //accept incoming client
        c_info.__clientfd = secure_accept(s_info.__serverfd, (struct sockaddr*)&c_info.__clientaddr, &c_info.__clientlen);
        puts("New client connected.");

        //send department name

        //create the thread to manage new connected client
        secure_pthread_create(&manager_tid, NULL, thread_client, &c_info.__clientfd);

    }

}
