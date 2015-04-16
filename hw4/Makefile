
# -g : allows use of GNU Debugger
# -Wall : show all warnings
FLAGS = -g -Wall
LIBS = # None yet...
SOURCE = main.c
OUTPUT = main

magic: $(SOURCE)
	@ # Call the compiler with source & output arguments
	$(CC) $(LIBS) $(FLAGS) -o $(OUTPUT) $(SOURCE)
	@ # Make the output file executable
	@chmod 755 $(OUTPUT)
	
test: $(SOURCE)
	@make clean
	make magic
	# Test 1: six westbound cars are sent, then two eastbound cars,  the last car to cross should be westbound
	./$(OUTPUT) WWWWWWEE
	#
	#
	# Test 2: one eastbound car is send, then seven westbound cars, then an eastbound car.  The last car should be make it just in time to cross
	./$(OUTPUT) E1WWWWWWWE
	#
	#
	# Test 3: alternates sending west and east cars, all the west cars should go, then all the eastbound ones
	./$(OUTPUT) WEWEWEWE
	#
	#
	# Test 4: misc test
	./$(OUTPUT) eeeee1w1e9w
	#
	#
	# Test 5: misc test
	./$(OUTPUT) eee1www3ew
# 'clean' rule for remove non-source files
# To use, call 'make clean'
# Note: you don't have to call this in between every
# compilation, it's only in case you need to
# clean out your directory for some reason.
clean:
	@ # Using the '@' sign suppresses echoing
	@ # the line while the command is run
	@rm -f $(OUTPUT)
