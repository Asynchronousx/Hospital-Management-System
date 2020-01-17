//This is our Doctor. He can retrieve all the information about specific department.

#include "Doctor_header.h"

int main() {

    //declaring variables
    connectioninfo info;
    serverinfo s_info; 
    int choice;
    

    //declaring name buffer
    char name[MAXBUFSIZE];
    char surname[MAXBUFSIZE];

    //fetching server info 
    serverConfig(&info.__port, info.__ip);

    //getting user generalities
    getUserInfo(name, surname);

    hospitalPrint();
    printf("Welcome, Dr. %s %s!\n", name, surname);
    puts("");
    hospitalMenu();

    //choosing
    puts("");
    printf("Choose: ");
    scanf("%d", &choice);
    getchar(); //remove newline

    if(choice == 1) {
        system("clear");
        printf("Hospital Management System -> Get Department visit info.\n\n");
        establishConnection(info, &s_info);
        getVisitInfo(s_info.__serverfd);
    } 

    if(connected) {
        close(s_info.__serverfd);
    }

    printf("Goodbye and Thank you for using our System!\n");
    
}

