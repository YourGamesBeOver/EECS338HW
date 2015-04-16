#include "utils.h"

void setColor(int attr, int fg, int bg){
	printf("%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
}

void resetColor(){
	setColor(RESET, WHITE, BLACK);
}
