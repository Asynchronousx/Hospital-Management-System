#MAKEFILE FOR USERCLIENT

PROGNAME = DoctorClient
OBJECTS  = DoctorClient.o secure_networking_functions.o Doctor_function.o
CFLAGS = -g
LIBS = -lpthread

$(PROGNAME): $(OBJECTS)
	gcc $(CFLAGS) -o $(PROGNAME) $(OBJECTS) $(LIBS) 
	rm -f $(OBJECTS)

# Clean Rule
clean: 
	rm -f $(OBJECTS) $(PROGNAME)
