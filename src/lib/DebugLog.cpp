#include "lib/DebugLog.h"

char buff1[1000];
char buff2[1000];


char* GetDebugBuffer1() {
	return buff1;
}
int GetDebugBuffer1Size() {
	return sizeof(buff1);
}
char* GetDebugBuffer2() {
	return buff2;
}
int GetDebugBuffer2Size() {
	return sizeof(buff2);
}
