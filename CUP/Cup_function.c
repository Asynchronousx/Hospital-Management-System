//File to store cup related functions

#include "Cup_header.h"

//------ Thread Function ------
void* thread_client(void* arg) {

    //declaring variables
    int client_fd = *(int*)arg;
    int stringlen;
    int greetingstrlen;
    int choice = 0; //initially no choice's made

    //socket info
    sockinfo socket_info;

    //generating structure
    socket_info.sock_array = &client_fd;
    socket_info.size = 1;

    //buffer
    char ID[MAXIDSIZE];
    char* greeting = "Connected to CUP Server.";

    //greeting strlen
    greetingstrlen = strlen(greeting);

    //while the users doesn't want to exit
    while(choice < 4) {

        //cleaning the buffer
        memset(ID, 0, MAXIDSIZE);

        //sending greeting size and greeting
        full_secure_send_with_ack(client_fd, &greetingstrlen, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        full_secure_send_with_ack(client_fd, greeting, greetingstrlen, MSG_NOSIGNAL, SERVER, &socket_info);

        //receiving identifier
        //receiving id size and identifier
        full_secure_recv_with_ack(client_fd, &stringlen, sizeof(int), 0, SERVER, &socket_info);
        full_secure_recv_with_ack(client_fd, ID, stringlen, MSG_NOSIGNAL, SERVER, &socket_info);

        if(strcmp(ID, "CLIENT") == 0) {
            //get the desired client activity
            full_secure_recv_with_ack(client_fd, &choice, sizeof(int), 0, SERVER, &socket_info);

            switch(choice) {
                case 1:
                    //manage the client schedule
                    manage_client_schedule(client_fd);
                    break;

                case 2:
                    //manage the client info
                    manage_client_info(client_fd);
                    break;
            
                case 3:
                    //manage the client delete
                    manage_client_delete(client_fd);
                    break;

                default:
                    //action not recognized: close the server
                    close(client_fd);
                    pthread_exit(NULL);
                    break;

            }

        } 
        else if(strcmp(ID, "EOC") == 0) {
            fprintf(stderr, "Client disconnected properly, thread exiting..\n");
            pthread_exit(NULL);
        }
        else {
            fprintf(stderr, "Client disconnected abnormaly, thread exiting..\n");
            pthread_exit(NULL);
        }
    }
}

//------ Client Managing ------

void manage_client_schedule(int c_fd) {

    //declaring the server info for the department
    departmentinfo* dep_info = NULL;
    sockinfo socket_info;
    int choice = 1;
    int hourchoice;
    int status;
    int size;

    //declaring buffer to store visits
    char parsed_generalities[MAXBUFSIZE*2]; //128
    char visits[MAXBUFSIZE*4]; //128
    char date[MINBUFSIZE*2]; //16
    char prencode[MINBUFSIZE*2];

    //cleaning
    memset(parsed_generalities, 0, sizeof(parsed_generalities));
    memset(visits, 0, sizeof(visits));
    memset(date, 0, sizeof(date));
    memset(prencode, 0, sizeof(prencode));

    //if the dep_addr if NULL, some error happened: go back.
    if((dep_info = get_department(c_fd, &dep_list, &mutex)) == NULL) {
        return;
    }

    //check if the department is still online
    if((status = try_department(&dep_info, &dep_list, c_fd, "CUP", &mutex)) == -1) {
        //if not, return to serve the client in another action
        return;
    }

    //alloc space for both client socket and department socket
    socket_info.sock_array = (int*)malloc(2*sizeof(int));

    //inserting at 0 client socket, 1 dep socket
    socket_info.sock_array[0] = c_fd;
    socket_info.sock_array[1] = dep_info->__depinfo.__serverfd;

    //inserting the size 
    socket_info.size = sizeof(socket_info.sock_array) / sizeof(socket_info.sock_array[0]);

    //once we got the department fd, we need to contact the department
    //choosen to inform on the choice we made.
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &choice, sizeof(int), 0, SERVER, &socket_info);

    //before receiving and sending the visit list to the department, we must wait until the user
    //made the choice of month/day, because he can choose to go back.
    //check if the client disconnected or nullified the actions: default -> disc
    status = TERM;
    full_secure_recv_with_ack(c_fd, &status, sizeof(int), 0, SERVER, &socket_info);

    //if the client nullified the action/disc
    if(status == TERM) {
        printf("Client went back on month choice, returning\n");
        //notify the dep
        full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //exit
        return;
    }

    //notify the department the client want the visit list
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

    //get the visit buffer size and the buffer 
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, visits, size, 0, SERVER, &socket_info);

    //send the size and the buffer to the client 
    full_secure_send_with_ack(c_fd, &size, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
    full_secure_send_with_ack(c_fd, visits, size, MSG_NOSIGNAL, SERVER, &socket_info);
    
    //receive date and time to send to the department
    //time choice
    full_secure_recv_with_ack(c_fd, &hourchoice, sizeof(int), 0, SERVER, &socket_info);

    //check date not nullified
    if(hourchoice == TERM) {
        printf("Client went back on hour choice, returning\n");
        //notify the dep
        status = hourchoice;
        full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //exit
        return;
    }

    //date size
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER, &socket_info);
    //date buff
    full_secure_recv_with_ack(c_fd, date, size, 0, SERVER, &socket_info);

    //sending the result to the department
    //time choice, date size and date buffer
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &hourchoice, sizeof(int), 0, SERVER, &socket_info);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, date, size, 0, SERVER, &socket_info);

    //wait for the department response: 
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), 0, SERVER, &socket_info);

    //if the status is 0 (failed)
    if(status == FAIL) {
        //notify the client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    //else if, the record was found
    } else if(status == FOUND) {
        //notify the client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    }

    //send the NOTFOUND status to the client
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

    //receive parsed generalities from the client
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_recv_with_ack(c_fd, parsed_generalities, size, 0, SERVER, &socket_info);

    //generate a prenotation code
    sprintf(prencode, "%s", generate_pren_code());

    //update generalities with pren code
    size = strlen(parsed_generalities);
    sprintf(parsed_generalities+size, ".%s.", prencode);

    //send the generalities to the department and register the visit
    size = strlen(parsed_generalities);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, parsed_generalities, size, 0, SERVER, &socket_info);

    //get status of the visit registration operation
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), 0, SERVER, &socket_info);

    //check status
    if(status == FAIL) {
        //send status to client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    }

    //if not failed send the success status to the client
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

    //send the prenotation code to the client
    size = strlen(prencode);
    full_secure_send_with_ack(c_fd, &size, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
    full_secure_send_with_ack(c_fd, prencode, size, MSG_NOSIGNAL, SERVER, &socket_info); 

    //done, close dep fd and return
    close(dep_info->__depinfo.__serverfd);

}

void manage_client_info(int c_fd) {

    //declaring the server info for the department
    departmentinfo* dep_info = NULL;
    sockinfo socket_info;
    int status;
    int size;
    int choice = 2;

    //declaring buffer
    char prenotation_info[MAXBUFSIZE*2]; //128
    char prenotation_code[BUFSIZE/2]; //16

    //cleaning
    memset(prenotation_info, 0, sizeof(prenotation_info));
    memset(prenotation_code, 0, sizeof(prenotation_code));

    //if the dep_addr if NULL, some error happened: go back.
    if((dep_info = get_department(c_fd, &dep_list, &mutex)) == NULL) {
        return;
    }
    
    //check if the department is still online
    if((status = try_department(&dep_info, &dep_list, c_fd, "CUP", &mutex)) == -1) {
        //if not, return to serve the client in another action
        return;
    }

    //alloc space for both client socket and department socket
    socket_info.sock_array = (int*)malloc(2*sizeof(int));

    //inserting at 0 client socket, 1 dep socket
    socket_info.sock_array[0] = c_fd;
    socket_info.sock_array[1] = dep_info->__depinfo.__serverfd;

    //inserting the size 
    socket_info.size = sizeof(socket_info.sock_array) / sizeof(socket_info.sock_array[0]);

    //once we got the department fd, we need to contact the department
    //choosen to inform on the choice we made.
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &choice, sizeof(int), 0, SERVER, &socket_info);

    //receive prenotation code
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_recv_with_ack(c_fd, prenotation_code, size, 0, SERVER, &socket_info);

    //send the code to the department
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, prenotation_code, size, 0, SERVER, &socket_info);

    //receive the status of the operation
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), 0, SERVER, &socket_info);

    //check status
    if(status == FAIL) {
        //send status to client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    } else if(status == NOTFOUND) {
        //notify the client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    }

    //else notify the client of the success of the operation
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

    //receive info from the department
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, prenotation_info, size, 0, SERVER, &socket_info);

    //send info to the user
    full_secure_send_with_ack(c_fd, &size, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
    full_secure_send_with_ack(c_fd, prenotation_info, size, MSG_NOSIGNAL, SERVER, &socket_info);

    //done, close dep fd and return
    close(dep_info->__depinfo.__serverfd);

}

void manage_client_delete(int c_fd) {

    //declaring the server info for the department
    departmentinfo* dep_info = NULL;
    sockinfo socket_info;
    int status;
    int size;
    int choice = 3;

    //declaring buffer
    char prenotation_code[BUFSIZE/2]; //16

    //cleaning
    memset(prenotation_code, 0, sizeof(prenotation_code));

    //if the dep_addr if NULL, some error happened: go back.
    if((dep_info = get_department(c_fd, &dep_list, &mutex)) == NULL) {
        return;
    }

    //check if the department is still online
    if((status = try_department(&dep_info, &dep_list, c_fd, "CUP", &mutex)) == -1) {
        //if not, return to serve the client in another action
        return;
    }

    //alloc space for both client socket and department socket
    socket_info.sock_array = (int*)malloc(2*sizeof(int));

    //inserting at 0 client socket, 1 dep socket
    socket_info.sock_array[0] = c_fd;
    socket_info.sock_array[1] = dep_info->__depinfo.__serverfd;

    //inserting the size 
    socket_info.size = sizeof(socket_info.sock_array) / sizeof(socket_info.sock_array[0]);

    //once we got the department fd, we need to contact the department
    //choosen to inform on the choice we made.
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &choice, sizeof(int), 0, SERVER, &socket_info);

    //get the prenotation code
    full_secure_recv_with_ack(c_fd, &size, sizeof(int), 0, SERVER, &socket_info);
    full_secure_recv_with_ack(c_fd, prenotation_code, size, 0, SERVER, &socket_info);

    //send the prenotation code to the department
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, &size, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
    full_secure_send_with_ack(dep_info->__depinfo.__serverfd, prenotation_code, size, MSG_NOSIGNAL, SERVER, &socket_info);

    //wait for the status of the operation
    full_secure_recv_with_ack(dep_info->__depinfo.__serverfd, &status, sizeof(int), 0, SERVER, &socket_info);

    //check error for the status received
    if(status == FAIL) {
        //send status to client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    } else if(status == NOTFOUND) {
        //send status to the client
        full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);
        //close connection with dep
        close(dep_info->__depinfo.__serverfd);
        //return
        return;
    }

    //if here, success: send the success status
    full_secure_send_with_ack(c_fd, &status, sizeof(int), MSG_NOSIGNAL, SERVER, &socket_info);

}

//------ Code generator ------

char* generate_pren_code() {

    char* code;
    int stringlen;
    char buffer[BUFSIZE];
    
    //cleaning
    memset(buffer, 0, BUFSIZE);

    //randomizing the seed
    srand(time(NULL));

    //generating random code
    for(int i=0; i<8; i++) {
            if(rand()%2) {
                buffer[i] = 'A' + (rand() % 26);
            } else {
                buffer[i] = rand() % 10 + '0';
            }
    }

    //returning the code
    buffer[8] = '\0'; 
    stringlen = strlen(buffer);
    code = (char*)malloc(stringlen);
    memcpy(code, buffer, stringlen);
    
    //return
    return code;

}