//Header for storing department related function prototype

#include "secure_networking.h"

void* thread_client(void*);
void get_user_prenotation(int c_fd);
void send_user_info(int c_fd);
void delete_user_prenotation(int c_fd);
void send_doctor_info(int c_fd);
void parse_generalities(char*, visitinfo*);

//global READ variables
databaseinfo globaldb_info;
char** globallist;