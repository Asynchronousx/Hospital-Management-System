//File to store DEPARTMENT related functions

#include "Department_header.h"

//------ Thread Client ------
void* thread_client(void* arg) {

    //getting the client fd
    int client_fd = *(int*)arg;

    //declaring variables
    int stringlen;
    int choice;
    int greetingstatus = 1;

    //declaring buffer
    char ID[MAXIDSIZE];

    //we need now to understand the ID of the connected client:
    //get ID size and ID.
    full_secure_recv_with_ack(client_fd, &stringlen, sizeof(int), 0, SERVER);
    full_secure_recv_with_ack(client_fd, ID, stringlen, 0, SERVER);

    //manage the id 
    //case 1: ID is TRYCON. This is sent by the CUP server
    //when the check of the server status occurr.
    if(strcmp(ID, "TRYCON") == 0) {
        //this is the thread that manage the connect try to make sure the server
        //is still online.
        close(client_fd);
    } 
    //case 2: ID is CUP. the CUP server want to perform an action.
    else if(strcmp(ID, "CUP") == 0) {
        //send greeting status and retrieve the choice.
        full_secure_send_with_ack(client_fd, &greetingstatus, sizeof(int), MSG_NOSIGNAL, SERVER);
        full_secure_recv_with_ack(client_fd, &choice, sizeof(int), 0, SERVER);

        switch(choice) {
            case 1:
                get_user_prenotation(client_fd);
                break;
            case 2:
                send_user_info(client_fd);
                break;
            case 3:
                delete_user_prenotation(client_fd);
                break;
            default:
                break;
        }
        
        close(client_fd);
    }
    //case 3: ID is DOC. the Doctor user want to retrieve information
    //about the visits.
    else if(strcmp(ID, "DOC") == 0) {
        //generating greeting
        char greeting[MAXBUFSIZE];
        sprintf(greeting, "Connected to %s server.", globaldb_info.__dbname);
        stringlen = strlen(greeting);

        //sending greeting
        full_secure_send_with_ack(client_fd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER);
        full_secure_send_with_ack(client_fd, greeting, stringlen, MSG_NOSIGNAL, SERVER);

        //sending infos
        send_doctor_info(client_fd);

        //close the descriptor
        close(client_fd);
    } 
    //case default: not recognized, exit.
    else {
        close(client_fd);
    }

    //EXIT 
    pthread_exit(NULL);
    
}

//------ User Interaction  -------
void get_user_prenotation(int c_fd) {

    //declaring variables
    int status;
    int hourchoice;
    int size;
    int result;
    int listsize = sizeof(globallist);
    int fixedsize = 4;
    int visitsent = 0;
    int stringlen = 0;

    //buffer
    char query[MAXBUFSIZE*4]; //256
    char visits[MAXBUFSIZE*4]; //256
    char date[MINBUFSIZE*2]; //16
    char parsed_generalities[MAXBUFSIZE*2]; //128

    //visit infos
    visitinfo user_info;

    //clean
    memset(parsed_generalities, 0, MAXBUFSIZE*2);
    memset(query, 0, MAXBUFSIZE*4);
    memset(visits, 0, MAXBUFSIZE*4);
    memset(date, 0, MINBUFSIZE*2);

    //for the entire size of the list 
    for(int i=0; i<=listsize; i++) {
        //print all the content into the buffer
        stringlen = strlen(visits);
        sprintf(visits+stringlen, "%d. \033[34m%s\033[0m\n", i+1, globallist[i]);
    }

    //getting the size
    stringlen = strlen(visits);

    //check if the client disconnected or nullified the actions before sending
    //the list.
    full_secure_recv_with_ack(c_fd, &status, sizeof(int), 0, SERVER);

    //check if successful or not
    if(status == TERM) {
        //nothing more to do, go back and close
        printf("Client went back on month choice, returning\n");
        return;
    }

    //send the size of the buffer to the cup server and the entire buffer 
    full_secure_send_with_ack(c_fd, &stringlen, sizeof(int), MSG_NOSIGNAL, SERVER);
    full_secure_send_with_ack(c_fd, visits, stringlen, MSG_NOSIGNAL, SERVER);

    //receive the date and time choosed by the user
    full_secure_recv_with_ack(c_fd, &hourchoice, sizeof(int), 0, SERVER);

    //check if user nullified
    if(hourchoice == TERM) {
        //nothing more to do, go back and close
        printf("Client went back on hour choice, returning\n");
        return;
    } else if ((hourchoice-1) > sizeof(globallist)) {
        //client selected an invalid hour; go back
        printf("Client selected an invalid hour, returning\n");
        return;
    }

    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER);
    full_secure_recv_with_ack(c_fd, date, size, 0, SERVER);
    
    //connect to db and creating our dbinfo struct
    databaseinfo thread_db_info;
    strcpy(thread_db_info.__dbname, globaldb_info.__dbname);
    strcpy(thread_db_info.__dbuser, globaldb_info.__dbuser);
    strcpy(thread_db_info.__dbuspws, globaldb_info.__dbuspws);

    //connect to db
    MYSQL* con = safe_db_connect(thread_db_info);

    //if the connection failed
    if(con == NULL) {
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //generate the query
    sprintf(query, "SELECT * FROM VISIT WHERE P_DATE='%s' AND P_TIME='%s';", date, globallist[hourchoice-1]);
    
    //query the database with the generated query (RETURN: 0 SUCCESS NONZERO FAILS)
    if(mysql_query(con, query)) {
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return; 
    }

    MYSQL_RES* res = mysql_store_result(con);
    MYSQL_ROW row;

    //check for errors
    if(res == NULL) {
        status = FAIL;
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //check if an entry already exists
    if(row = mysql_fetch_row(res)) {
        status = FOUND;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        //exit
        return;
    } else {
        status = NOTFOUND;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
    }

    
    //wait for user generalities
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER);
    full_secure_recv_with_ack(c_fd, parsed_generalities, size, 0, SERVER);

    //parse generalities 
    parse_generalities(parsed_generalities, &user_info);

    //updating time and date 
    strcpy(user_info.__date, date);
    strcpy(user_info.__time, globallist[hourchoice-1]);

    //insert the record into the database
    status = insert_into_database(&user_info, con);

    //send status to server
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);

    //done, free connection and return
    mysql_close(con);
    mysql_free_result(res);

}

void send_user_info(int c_fd) {
    
    //declaring variables
    int status;
    int size;

    //declaring buffer
    char prenotation_code[BUFSIZE/2]; //16
    char query[MAXBUFSIZE];

    //cleaning
    memset(prenotation_code, 0, sizeof(prenotation_code));
    memset(query, 0, MAXBUFSIZE);

    //receive prenotation code
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER);
    full_secure_recv_with_ack(c_fd, prenotation_code, size, 0, SERVER);

    //building the query
    sprintf(query, "SELECT * FROM VISIT WHERE PREN_CODE = '%s';", prenotation_code);

    //opening db
    databaseinfo thread_db_info;
    strcpy(thread_db_info.__dbname, globaldb_info.__dbname);
    strcpy(thread_db_info.__dbuser, globaldb_info.__dbuser);
    strcpy(thread_db_info.__dbuspws, globaldb_info.__dbuspws);
    MYSQL* con = safe_db_connect(thread_db_info);

    //checking status of operation
    if(con == NULL) {
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //retrieving the info
    char* prenotation_info = getinfo_from_database(query, con);

    //status checking
    if(strcmp(prenotation_info, "FAIL") == 0) {
        //if failed
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    } 
    else if(strcmp(prenotation_info, "NOTFOUND") == 0) {
        status = NOTFOUND;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //if here, the operation was successful: send success status
    status = SUCCESS;
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);

    //send the info
    size = strlen(prenotation_info);
    full_secure_send_with_ack(c_fd, &size, sizeof(int), MSG_NOSIGNAL, SERVER);
    full_secure_send_with_ack(c_fd, prenotation_info, size, MSG_NOSIGNAL, SERVER);

    //done, free connection and return
    free(prenotation_info);
    mysql_close(con);

}

void delete_user_prenotation(int c_fd) {
    
    //declaring variables
    int status;
    int size;

    //declaring buffer
    char prenotation_code[BUFSIZE/2]; //16
    char query[MAXBUFSIZE];

    //cleaning
    memset(prenotation_code, 0, sizeof(prenotation_code));
    memset(query, 0, MAXBUFSIZE);

    //receive prenotation code
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER);
    full_secure_recv_with_ack(c_fd, prenotation_code, size, 0, SERVER);

    //building the query
    sprintf(query, "SELECT * FROM VISIT WHERE PREN_CODE = '%s';", prenotation_code);

    //opening db
    databaseinfo thread_db_info;
    strcpy(thread_db_info.__dbname, globaldb_info.__dbname);
    strcpy(thread_db_info.__dbuser, globaldb_info.__dbuser);
    strcpy(thread_db_info.__dbuspws, globaldb_info.__dbuspws);
    MYSQL* con = safe_db_connect(thread_db_info);

    //checking status of operation
    if(con == NULL) {
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //going to delete the entry
    int cancel_status = cancel_from_database(query, con, prenotation_code);

    //status checking
    if(cancel_status == FAIL) {
        //notify the cup
        full_secure_send_with_ack(c_fd, &cancel_status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    } 
    else if(cancel_status == NOTFOUND) {
        //notify the cup
        full_secure_send_with_ack(c_fd, &cancel_status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //if here, the operation was successful: send success status
    full_secure_send_with_ack(c_fd, &cancel_status, sizeof(int), MSG_NOSIGNAL, SERVER);

    //dealloc
    mysql_close(con);
}

void send_doctor_info(int c_fd) {

    //delcaring variables
    int status;

    //declaring buffer
    char query[MAXBUFSIZE*2];

    //cleaning
    memset(query, 0, MAXBUFSIZE*2);

    //opening db
    databaseinfo thread_db_info;
    strcpy(thread_db_info.__dbname, globaldb_info.__dbname);
    strcpy(thread_db_info.__dbuser, globaldb_info.__dbuser);
    strcpy(thread_db_info.__dbuspws, globaldb_info.__dbuspws);
    MYSQL* con = safe_db_connect(thread_db_info);

    if(con == NULL) {
        status = FAIL;
        //notify the cup
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER);
        return;
    }

    //generating query
    sprintf(query, "SELECT P_DAY, P_DATE, P_TIME, P_FNAME, P_LNAME FROM VISIT ORDER BY P_DAY;");

    //sending the infos
    show_department_visits(c_fd, query, con);

    //can return
    mysql_close(con);

}


//------ Parsing generalities ------
void parse_generalities(char* generalities, visitinfo* user_info) {

    //generalities are in form of:
    //name, surname, recept, prenotation code.

    //declaring variables
    int count = 0;
    int parse_case = 0;
    int stringlen = strlen(generalities);

    //declaring buffer 
    char buffer[BUFSIZE];

    //cleaning
    memset(buffer, 0, BUFSIZE);

    //parsing
    for(int i=0; i<stringlen; i++) {
        if(generalities[i] != '.') {
            buffer[count] = generalities[i];
            count++;
        } else {
            //switch the parse case
            switch(parse_case) {
                case 0: //is name
                    strcpy(user_info->__pfname, buffer);
                    break;

                case 1: //is surname
                    strcpy(user_info->__plname, buffer);
                    break;
                
                case 2: //is recept
                    strcpy(user_info->__recept, buffer);
                    break;
                
                case 3: //is pren code
                    strcpy(user_info->__prenotationcode, buffer);
                    break;

                default: 
                    break;
            }

            //reset count and buffer
            count = 0;
            memset(buffer, 0, BUFSIZE);

            //increase parse case
            parse_case++;

        }
    }
}
