//Header for storing cup related function prototype

#include "secure_networking.h"

void* thread_client(void*);
void manage_client_schedule(int);
void manage_client_info(int);
void manage_client_delete(int);
char* generate_pren_code();

//global department list
departmentlist* dep_list;

//mutex semaphore
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;