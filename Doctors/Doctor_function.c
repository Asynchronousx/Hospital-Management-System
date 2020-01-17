//File to store doctor related functions

#include "Doctor_header.h"

//------ Get user infos ------
void getUserInfo(char* name, char* surname) {
    hospitalPrint();
    puts("");
    puts("Enter your information!");
    scan_string(name, "Name", 2, 32);
    scan_string(surname, "Surname", 2, 32);
}

//------ View Controller Functions ------
void hospitalPrint() {
    system("clear");
    printf("**************************************\n");
    printf("* \033[22;34mWelcome to the Hospital Management\033[0m *\n");
    printf("*               \033[22;34mSystem!\033[0m              *\n");
    printf("**************************************\n");
}

void hospitalMenu() {

    printf("What would you like to do?\n");
    puts("");
    printf("1. Get department visit info.\n");
    printf("2. Exit.");
    puts("");

}

//------ Get visit infos ------
void getVisitInfo(int s_fd) {

    //declaring variables
    int status;
    int size;
    int choice;
    FILE* file;

    //declaring buffer
    char day_of_week[BUFSIZE];
    char day_check[BUFSIZE];
    char visit_info[MAXBUFSIZE*2];

    //cleaning
    memset(day_of_week, 0, BUFSIZE);
    memset(day_check, 0, BUFSIZE); 
    memset(visit_info, 0, MAXBUFSIZE*2);

    puts("");
    printf("Memorize visits into a file? (\033[32m1-YES\033[0m / \033[31m0-NO\033[0m): ");
    while(choice > 1 || choice < 0) {
        scanf("%d", &choice);
        if(choice > 1 || choice < 0) {
            printf("Choice not recognized. Try again.\n");
            printf("(1 YES / 0 NO): ");
        }
    }

    if(choice) {
        file = openFile();
    }

    puts("");
    printf("Retrieving visit info from the department..\n");

    //receive status
    full_secure_recv_with_ack(s_fd, &status, sizeof(int), 0, CLIENT);

    if(status == FAIL) {
        puts("");
        puts("\033[31mSomething went wrong. Please try again.\033[0m\n");
        puts("");
        return;
    }
    else if(status == NOTFOUND) {
        puts("");
        puts("\033[31mSorry, no result found into this department. Try later.\033[0m\n");
        puts("");
        return;
    }

    printf("Data retrieving: ");
    puts("");
    while(full_secure_recv_with_ack(s_fd, &size, sizeof(int), 0, CLIENT)) {

        //check for day of week or EOL from server
        full_secure_recv_with_ack(s_fd, &day_check, size, 0, CLIENT);

        //compare to already seen day of week or EOL
        if(strcmp(day_check, "EOL") == 0) {
            //if end of list, break
            puts("");
            printf("------ \033[32mEnd of List\033[0m ------\n");
            fprintf(file, "\n");
            fprintf(file, "------ End of List ------");
            fprintf(file, "\n");
            break;
        }
        else if(strcmp(day_check, day_of_week) != 0) {
            //if the day sent differnt from the last one, reset, copy and print it out
            memset(day_of_week, 0, BUFSIZE);
            strcpy(day_of_week, day_check);
            puts("");
            printf("------ \033[32m%s\033[0m ------\n", day_of_week);
            fprintf(file, "\n");
            fprintf(file, "------ %s ------", day_of_week);
            fprintf(file, "\n");

        }

        //now receive the record
        full_secure_recv_with_ack(s_fd, &size, sizeof(int), 0, CLIENT);
        full_secure_recv_with_ack(s_fd, visit_info, size, 0, CLIENT);

        //print the record
        printf("%s\n", visit_info);
        fprintf(file, "%s", visit_info);
        fprintf(file, "\n");

        //reset
        memset(day_check, 0, BUFSIZE);
        memset(visit_info, 0, MAXBUFSIZE*2);

    }


    getchar(); 
    puts("");
    printf("Press <ENTER> to return.");
    int c;
    while((c = getchar()) != '\n' && c != EOF);
    puts("");

    //done

}

//------ Connection to server ------
void establishConnection(connectioninfo info, serverinfo* s_info) {

    //declaring buffer
    char greeting[MAXBUFSIZE];
    char ack[MINBUFSIZE];
    char* ID = "DOC";
    int stringlen;

    //cleaning buffer
    memset(greeting, 0, MAXBUFSIZE);
    memset(ack, 0, MINBUFSIZE);

    //initialization of the server
    if(!connected) {
        secure_server_init(info, s_info);

        //connection phase
        puts("Connection to the server..");
        secure_connect(s_info->__serverfd, (struct sockaddr*)&s_info->__serveraddr, s_info->__serverlen);

        //connected
        connected = true;
    }

    //assigning the returning FD to the global one (in case of SIGINT close it)
    s_fd = s_info->__serverfd;

    //seding identifier
    stringlen = strlen(ID);
    full_secure_send_with_ack(s_fd, &stringlen, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(s_fd, ID, stringlen, 0, CLIENT);

    //receive greeting size and greeting
    full_secure_recv_with_ack(s_fd, &stringlen, sizeof(int), 0, CLIENT);
    full_secure_recv_with_ack(s_fd, greeting, stringlen, 0, CLIENT);

    
    //print greeting
    printf("%s\n",greeting);

}

//------ File opening ------
FILE* openFile(void) {

    //declaring variables
    FILE* file;

    //opening the file into the directory
    //if not found, create it
    file = fopen("./VisitInfo/visit_info.txt", "w+");

    return file;

}