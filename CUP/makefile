#MAKEFILE FOR USERCLIENT

PROGNAME = CUP
OBJECTS  = CUP.o secure_networking_functions.o Cup_function.o
CFLAGS = -g 
LIBS = -lpthread

$(PROGNAME): $(OBJECTS)
	gcc -o $(PROGNAME) $(OBJECTS) $(LIBS)
	rm -f $(OBJECTS)

# Clean Rule
clean: 
	rm -f $(OBJECTS) $(PROGNAME)
