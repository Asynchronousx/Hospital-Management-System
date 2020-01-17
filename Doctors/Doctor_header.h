//Header for storing doctor related function prototype

#include "secure_networking.h"

void getUserInfo(char*, char*);
void hospitalPrint(void);
void hospitalMenu(void);
void getVisitInfo(int);
FILE* openFile();
void establishConnection(connectioninfo, serverinfo*);

//global var
static bool connected = false;
int s_fd;