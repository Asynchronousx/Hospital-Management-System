//File to store user related functions

#include "User_header.h"

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
    printf("1. Schedule visit.\n");
    printf("2. Get visit information.\n");
    printf("3. Cancel visit.\n");
    printf("4. Exit.\n");
    puts("");

}

void backToHospital() {

    printf("Press <ENTER> to return back.\n");
    while(getchar() != '\n');
    system("clear");
    
}


//------ Visit Management Function ------
//Booking a visit
void bookVisit(int serverfd, char* name, char* surname) {
    
    //declaring variables
    int status;
    int size;
    int hourchoice;
    int monthchoice;
    int daychoice;

    //declaring buffers
    char recept[BUFSIZE]; //32
    char date[MINBUFSIZE*2]; //16
    char visits[MAXBUFSIZE*4]; //256
    char parsed_generalities[MAXBUFSIZE*2]; //128
    char prenotation_code[MINBUFSIZE*2]; //16

    //visit info struct
    visitinfo my_info;

    //cleaning
    memset(recept, 0, sizeof(recept));
    memset(date, 0, sizeof(date));
    memset(visits, 0, sizeof(visits));
    memset(parsed_generalities, 0, sizeof(parsed_generalities));
    memset(prenotation_code, 0, sizeof(prenotation_code));

    //check if sending the department was good 
    if((status = send_department(serverfd, 1)) == -1) {
        backToHospital();
        return;
    }    

    //TODO: imeplement schedule

    //if the department was found offline
    if(status == OFFLINE) {
        puts("");
        puts("Selected Department is \033[31mOFFLINE.\033[0m");
        puts("Please, go back and refresh the page.\n");
        backToHospital();
        return;
    }


    //fetching the year
    time_t T= time(NULL);
    struct tm tm = *localtime(&T);
    int year = tm.tm_year+1900;
    
    //getting the recept
    puts("");
    scan_string(recept, "Please, insert your recept code", 15, 15);

    //choosing the month
    printMonth();
    puts("");

    while(monthchoice > 12 || monthchoice < 1) {
        scan_int("Choose your Month (MON. NUM or -1 to go back)", &monthchoice);        
        //if the user want to go back
        if(monthchoice == -1) {
            break;
        }
    }

    //check if the user want to go back
    if(monthchoice == -1) {
        monthchoice = TERM;
        full_secure_send_with_ack(serverfd, &monthchoice, sizeof(int), 0, CLIENT);
        backToHospital();
        return;
    }

    //choosing the day in base of the month
    selectDay(&daychoice, monthchoice);

    //all good, send the OK status to the server.
    status = 1;
    full_secure_send_with_ack(serverfd, &status, sizeof(int), 0, CLIENT);

    //receive from server the visit list size and the visit list buffer
    full_secure_recv_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_recv_with_ack(serverfd, visits, size, 0, CLIENT);

    //schedule availabilty
    puts("");
    printf("Available Prenotation Hours:\n");
    printf("%s", visits);
    puts("");

    scan_int("Choose your schedule: (SCHED. NUM or -1 to go back)", &hourchoice);

    if(hourchoice == -1) {
        hourchoice = TERM;
        full_secure_send_with_ack(serverfd, &hourchoice, sizeof(int), 0, CLIENT);
        int c = fgetc(stdin);
        puts("");
        backToHospital();
        return;
    }

    //sending all to the server: date and time
    //parsing date
    sprintf(date, "%d-%d-%d", year, monthchoice, daychoice);
    size = strlen(date);

    //sending date and time to the server to check availability
    full_secure_send_with_ack(serverfd, &hourchoice, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(serverfd, date, size, 0, CLIENT);

    //receive status from the cup server
    full_secure_recv_with_ack(serverfd, &status, sizeof(int), 0, CLIENT);
    
    if(status == FOUND) {
        puts("");
        printf("\033[31mSorry, date already scheduled. Go back and select another date.\033[0m\n");
        puts("");
        int c = fgetc(stdin);
        backToHospital();
        return;
    } 
    else if (status == FAIL) {
        puts("");
        puts("\033[31mSomething went wrong. Please try again.\033[0m\n");
        puts("");
        int c = fgetc(stdin);
        backToHospital();
        return;
    }

    //send parsed generalities to the server
    sprintf(parsed_generalities, "%s.%s.%s", name, surname, recept);
    size = strlen(parsed_generalities);
    full_secure_send_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(serverfd, parsed_generalities, size, 0, CLIENT);

    //receive status
    full_secure_recv_with_ack(serverfd, &status, sizeof(int), 0, CLIENT);

    //check status
    if(status == FAIL) {
        puts("");
        puts("\033[31mSomething went wrong. Please try again.\033[0m\n");
        puts("");
        backToHospital();
        int c = fgetc(stdin);
        backToHospital();
        return;
    }

    //receive prenotation code
    full_secure_recv_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_recv_with_ack(serverfd, prenotation_code, size, 0, CLIENT);

    puts("");
    printf("Prenotation scheduled correctly.\n");
    printf("Your prenotation code is: \033[32m%s\033[0m\n", prenotation_code);
    puts("");
    
    //clean the buffer before calling back to hospital:
    //that's because scanf make stdin dirty.
    int c = fgetc(stdin);

    //undo to hospital and close connection
    backToHospital();
}

//Get visit info
void getVisitInfo(int serverfd) {

    //declaring variables
    int status;
    int size;

    //declaring buffer
    char prenotation_info[MAXBUFSIZE*2]; //128
    char prenotation_code[BUFSIZE/2]; //16

    //cleaning
    memset(prenotation_info, 0, sizeof(prenotation_info));
    memset(prenotation_code, 0, sizeof(prenotation_code));

    //check if sending the department was good 
    if((status = send_department(serverfd, 2)) == -1) {
        backToHospital();
        return;
    }    

    //if the department was found offline
    if(status == OFFLINE) {
        puts("Selected Department is OFFLINE.");
        puts("Please, go back and refresh the page.\n");
        backToHospital();
        return;
    }
    
    //getting the prenotation code
    puts("");
    printf("Please, insert your prenotation code: ");
    fgets(prenotation_code, sizeof(prenotation_code), stdin);
    size = strlen(prenotation_code);
    prenotation_code[size-1] = '\0';

    //send the prenotation code to the server
    full_secure_send_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(serverfd, prenotation_code, size, 0, CLIENT);

    //receive the status of the operation
    full_secure_recv_with_ack(serverfd, &status, sizeof(int), 0, CLIENT);

    //check status
    if(status == NOTFOUND) {
        puts("");
        printf("\033[31mDidn't find anything with this prenotation code. Please try again.\033[0m\n");
        puts("");
        backToHospital();
        return;
    } 
    else if (status == FAIL) {
        puts("");
        puts("\033[31mSomething went wrong. Please try again.\033[0m\n");
        puts("");
        backToHospital();
        return;
    }

    //receive the prenotation info
    full_secure_recv_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_recv_with_ack(serverfd, prenotation_info, size, 0, CLIENT);

    puts("");
    printf("Prenotation info found:\n");
    printf("\033[32m%s\033[0m\n", prenotation_info);
    puts("");

    //undo to hospital and close connection
    backToHospital();

}

//Cancel an existing visit
void cancelVisit(int serverfd) {
    
    //declaring variables
    int status;
    int size;

    //declaring buffer
    char prenotation_code[BUFSIZE/2]; //16

    //cleaning
    memset(prenotation_code, 0, sizeof(prenotation_code));

    //check if sending the department was good 
    if((status = send_department(serverfd, 3)) == -1) {
        backToHospital();
        return;
    }    


    //TODO: imeplement cancel

    //if the department was found offline
    if(status == OFFLINE) {
        puts("Selected Department is OFFLINE.");
        puts("Please, go back and refresh the page.\n");
        backToHospital();
        return;
    }
    
    //sending the choice to the server

    //getting the prenotation code
    puts("");
    printf("Please, insert your prenotation code: ");
    fgets(prenotation_code, sizeof(prenotation_code), stdin);
    size = strlen(prenotation_code);
    prenotation_code[size-1] = '\0';

    //send the prenotation code to the server
    full_secure_send_with_ack(serverfd, &size, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(serverfd, prenotation_code, size, 0, CLIENT);

    //receive the status of the operation
    full_secure_recv_with_ack(serverfd, &status, sizeof(int), 0, CLIENT);

    //check status
    if(status == NOTFOUND) {
        puts("");
        printf("\033[31mDidn't find anything with this prenotation code. Please try again.\033[0m\n");
        puts("");
        backToHospital();
        return;
    } 
    else if (status == FAIL) {
        puts("");
        printf("\033[31mSomething went wrong. Please try again.\033[0m\n");
        puts("");
        backToHospital();
        return;
    }

    //confirm deletion
    puts("");
    printf("\033[32mPrenotation delete successfully.\033[0m\n");
    puts("");

    //undo to hospital and close connection
    backToHospital();
}


//------ Connection to server ------
void establishConnection(connectioninfo info, serverinfo* s_info) {

    //declaring buffer
    char greeting[MAXBUFSIZE];
    char ack[MINBUFSIZE];
    char* ID = "CLIENT";
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


    //connected, do things
    //receive greeting size and greeting
    full_secure_recv_with_ack(s_fd, &stringlen, sizeof(int), 0, CLIENT);
    full_secure_recv_with_ack(s_fd, greeting, stringlen, 0, CLIENT);

    //seding identifier
    stringlen = strlen(ID);
    full_secure_send_with_ack(s_fd, &stringlen, sizeof(int), 0, CLIENT);
    full_secure_send_with_ack(s_fd, ID, stringlen, 0, CLIENT);
    
    //print greeting
    printf("%s\n",greeting);

}

//TODO: INSERT MONTH AND DAY CHECK
void printMonth() {

    puts("");
    printf("Available Months:\n");
    printf("1.  \033[34mJanaury\033[0m\n");
    printf("2.  \033[34mFebruary\033[0m\n");
    printf("3.  \033[34mMarch\033[0m\n");
    printf("4.  \033[34mApril\033[0m\n");
    printf("5.  \033[34mMay\033[0m\n");
    printf("6.  \033[34mJune\033[0m\n");
    printf("7.  \033[34mJuly\033[0m\n");
    printf("8.  \033[34mAugust\033[0m\n");
    printf("9.  \033[34mSeptember\033[0m\n");
    printf("10. \033[34mOctober\033[0m\n");
    printf("11. \033[34mNovember\033[0m\n");
    printf("12. \033[34mDicember\033[0m\n");

}

void selectDay(int* daychoosed, int month) {

    int choice;
    bool good = false;

    while(!good) {
        switch(month) {
            case 1:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 2:
                scan_int("Select a day (1-28)", &choice);
                *daychoosed = choice;
                if(choice <= 28 && choice >= 1) 
                    good = true;
                break;

            case 3:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 4:
                scan_int("Select a day (1-30)", &choice);
                *daychoosed = choice;
                if(choice <= 30 && choice >= 1) 
                    good = true;
                break;

            case 5:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 6:
                scan_int("Select a day (1-30)", &choice);
                *daychoosed = choice;
                if(choice <= 30 && choice >= 1) 
                    good = true;
                break;

            case 7:
                scan_int("Select a day (1-30)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 8:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 9:
                scan_int("Select a day (1-30)", &choice);
                *daychoosed = choice;
                if(choice <= 30 && choice >= 1) 
                    good = true;
                break;

            case 10:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            case 11:
                scan_int("Select a day (1-30)", &choice);
                *daychoosed = choice;
                if(choice <= 30 && choice >= 1) 
                    good = true;
                break;

            case 12:
                scan_int("Select a day (1-31)", &choice);
                *daychoosed = choice;
                if(choice <= 31 && choice >= 1) 
                    good = true;
                break;

            default: 
                break;

        }

    if(!good) {
        puts("Day not recognized. Please select it again.");
    }

    }
}


