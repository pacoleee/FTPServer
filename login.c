#include <string.h>
#include <stdio.h>
#include "login.h"

int login(char* username) 
{
	return strcmp(username, "cs317") == 0; 
}