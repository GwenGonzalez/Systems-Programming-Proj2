/*********************************************
*        DO NOT REMOVE THIS MESSAGE
*
* This file is provided by Professor Littleton
* to assist students with completing Project 3.
*
*        DO NOT REMOVE THIS MESSAGE
**********************************************/

#include "headers.h"

#define SINGLE_QUOTE 39

// List of valid directives
enum directives {
	// Although ERROR is not a valid directive, 
	// its presence helps the isDirective() function
	ERROR, BASE, BYTE, END, RESB, RESW, START
};

// Returns the value associated with a BYTE directive
int getByteValue(int directiveType, char* string)
{
	if (string[0] == 'C') {
		char* hexString = malloc(2 * strlen(string) + 1);
		hexString[0] = '\0';

		for (int i = 0; i < strlen(string); i++) {
			char temp[3];
			if (string[i] == 39 || string[i] == 67) {
				continue;
			}
			sprintf(temp, "%x", string[i]);
			strcat(hexString, temp);
		}
		int hex = (int)strtol(hexString, NULL, 16);
		return hex;
		/*
		char hexString[50];
		string = &string[2];
		int i = 0;
		while (string[i] != '\'') {
			int ascii = string[i];
			char hex[10];
			sprintf(hex, "%x", ascii);
			strcat(hexString, hex);
			i++;
		}
		char* endptr;
		int hex = strtol(hexString, &endptr, 16);
		//FIX LATER
		return 60;
		*/
	}
	else {
		char opString[3];
		string = &string[2];
		for (int i = 0; i < 2; i++) {
			opString[i] = string[i];
		}
		opString[2] = '\0';	
		char* endptr;
		int hex = strtol(string, &endptr, 16);	
		return hex;
	}
	
}

// Do no modify any part of this function
// Returns the number of bytes required to store the BYTE directive value in memory
int getMemoryAmount(int directiveType, char* string)
{
	char hex[9] = { '\0' };
	int temp = 0;
	
	switch (directiveType)
	{
		case BASE:
		case END:
		case START:
			return 0;
			break;
		case BYTE:
			if (string[0] == 'X')
			{
				if (strlen(string) != 5)
				{
					displayError(OUT_OF_RANGE_BYTE, string);
					exit(-1);
				}
				else
					return 1;
			}
			else if (string[0] == 'C')
				return strlen(string) - 3;
			break;
		case RESB:
			return strtol(string, NULL, 10);
			break;
		case RESW:
			return strtol(string, NULL, 10) * 3;
			break;
	}
	return -1; // Should not happen
}

// Returns true if the provided directive type is the BASE directive; otherwise, false
bool isBaseDirective(int directiveType)
{
	return directiveType == BASE;
}

// Returns true if the provided directive type is the BYTE directive; otherwise, false
bool isDataDirective(int directiveType)
{
	return directiveType == BYTE;
}

// Do no modify any part of this function
// Tests whether the provided string is a valid directive
// Returns true if string is valid directive; otherwise, false
int isDirective(char* string) 
{
	if (strcmp(string, "BASE") == 0) { return BASE; }
	else if (strcmp(string, "BYTE") == 0) { return BYTE; }
	else if(strcmp(string, "END") == 0) { return END; }
	else if (strcmp(string, "RESB") == 0) { return RESB; }
	else if (strcmp(string, "RESW") == 0) { return RESW; }
	else if (strcmp(string, "START") == 0) { return START; }
	else { return ERROR; }
}

// Returns true if the provided directive type is the END directive; otherwise, false
bool isEndDirective(int directiveType)
{
	return directiveType == END;
}

// Returns true if the provided directive type is the RESB or RESW directive; otherwise, false
bool isReserveDirective(int directiveType)
{
	if (directiveType == RESB || directiveType == RESW) {
		return true;
	}
	else {
		return false;
	}
}

// Returns true if the provided directive type is the START directive; otherwise, false
bool isStartDirective(int directiveType)
{
	return directiveType == START;
}