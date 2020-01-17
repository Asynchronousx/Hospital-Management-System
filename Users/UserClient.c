//This is our User into the hospital database. 
//A user could Book, cancel or get info about a visit he prenoted. 

#include "User_header.h"

int main(void) {
    
    //declaring connectioninfo variable
    connectioninfo info;
    serverinfo s_info;

    //installing the signal handler for SIGPIPE
    signal(EPIPE, SIG_IGN);

    //declaring name buffer
    char name[BUFSIZE];
    char surname[BUFSIZE];

    //fetching data from the config file
    serverConfig(&info.__port, info.__ip);

    //getting name and surname of the user
    getUserInfo(name, surname);

    //declaring variables
    int choice;

    hospitalPrint();
    printf("USER: %s %s\n", name, surname);
    puts("");
    hospitalMenu();

    scan_int("Choose", &choice);
    getchar(); //remove newline

    while(choice != 4) {
        switch(choice) {
            case 1:
                system("clear");
                printf("Hospital Management System -> Schedule Visit.\n\n");
                establishConnection(info, &s_info);
                bookVisit(s_info.__serverfd, name, surname);
                break;

            case 2:
                system("clear");
                printf("Hospital Management System -> Get Visit Information.\n\n");
                establishConnection(info, &s_info);
                getVisitInfo(s_info.__serverfd);
                break;

            case 3:
                system("clear");
                printf("Hospital Management System -> Cancel visit.\n\n");
                establishConnection(info, &s_info);
                cancelVisit(s_info.__serverfd);
                break;

            default:
                system("clear");
                printf("Choice not recognized.\n");
                hospitalPrint();
                printf("USER: %s %s\n", name, surname);
                puts("");
                hospitalMenu();
                printf("Choose: ");
                scanf("%d", &choice);
                break;
        }

        hospitalPrint();
        printf("USER: %s %s\n", name, surname);
        puts("");
        hospitalMenu();
        scan_int("Choose", &choice);
        getchar();

    }

 //tell the server you're disconnecting
 if(connected) {
    char* disc = "EOC";
    int stringlen = strlen(disc);
    full_secure_send_with_ack(s_info.__serverfd, &stringlen, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(s_info.__serverfd, "EOC", stringlen, 0, CLIENT);
    close(s_info.__serverfd);
 }

    puts("");
    printf("Goodbye and Thank you for using our System!\n");

}
