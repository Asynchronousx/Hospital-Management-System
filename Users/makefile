#MAKEFILE FOR USERCLIENT

PROGNAME = UserClient
OBJECTS  = UserClient.o secure_networking_functions.o User_function.o
CFLAGS = -g
LIBS = -lpthread

$(PROGNAME): $(OBJECTS)
	gcc -o $(PROGNAME) $(OBJECTS) $(LIBS)
	rm -f $(OBJECTS)

# Clean Rule
clean: 
	rm -f $(OBJECTS) $(PROGNAME)
