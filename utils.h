#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

#include <stdio.h>

//utils.h
//contains some utilities for printing text to the console

#define RESET		0
#define BRIGHT 		1
#define DIM		2
#define UNDERLINE 	3
#define BLINK		4
#define REVERSE		7
#define HIDDEN		8

#define BLACK 		0
#define RED		1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define	WHITE		7




void setColor(int attr, int fg, int bg);
void resetColor();


#endif
