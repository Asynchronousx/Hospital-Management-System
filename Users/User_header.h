//Header for storing user related function prototype

#include "secure_networking.h"

void getUserInfo(char*, char*);
void hospitalPrint();
void hospitalMenu();
void backToHospital();
void establishConnection(connectioninfo, serverinfo*);
void bookVisit(int, char*, char*);
void getVisitInfo(int);
void cancelVisit(int);
void serverCommunicate(int);
void printMonth();
void selectDay(int*, int);
void switchStatus(int);
int error_check(int status, int* filedes);
static void handleSIGINT(int);

static bool connected = false;
int s_fd;