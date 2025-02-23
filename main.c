#include "headers.h"

// Pass 1 constants
#define COMMENT 35
#define INPUT_BUF_SIZE 60
#define NEW_LINE 10
#define SPACE 32
#define SYMBOL_TABLE_SIZE 100

// Pass 2 constants
#define BLANK_INSTRUCTION 0x000000
#define FLAG_B 0x04
#define FLAG_E 0x01
#define FLAG_I 0x10
#define FLAG_N 0x20
#define FLAG_P 0x02
#define FLAG_X 0x08
#define FORMAT_1 1
#define FORMAT_2 2
#define FORMAT_3 3
#define FORMAT_3_MULTIPLIER 0x1000
#define FORMAT_4 4
#define FORMAT_4_MULTIPLIER 0x100000
#define IMMEDIATE_CHARACTER '#'
#define INDEX_STRING ",X"
#define INDIRECT_CHARACTER '@'
#define MAX_RECORD_BYTE_COUNT 30
#define OPCODE_MULTIPLIER 0x100
#define OUTPUT_BUF_SIZE 70
#define REGISTER_A 0X0
#define REGISTER_B 0X3
#define REGISTER_L 0X2
#define REGISTER_MULTIPLIER 0x10
#define REGISTER_S 0X4
#define REGISTER_T 0X5
#define REGISTER_X 0X1
#define RSUB_INSTRUCTION 0x4C0000
#define BASE_MAX_RANGE 4096
#define PC_MAX_RANGE 2048
#define PC_MIN_RANGE -2048

// Pass 1 functions
void performPass1(symbol* symbolTable[], char* filename, address* addresses);
segment* prepareSegments(char* line);
void trim(char string[]);

// Pass 2 functions
int computeFlags(struct symbol* symbolArray[], address* addresses, segment* segments, int format);
char* createFilename(char* filename, const char* extension);
void flushTextRecord(FILE* file, objectFileData* data, address* addresses);
int getRegisters(char* operand);
int getRegisterValue(char registerName);
bool isNumeric(char* string);
void performPass2(struct symbol* symbolTable[], char* filename, address* addresses);
void writeToLstFile(FILE* file, int address, segment* segments, int opcode);
void writeToObjFile(FILE* file, objectFileData data);

int main(int argc, char* argv[])
{
	// Do not modify this statement
	address addresses = { 0x00, 0x00, 0x00 };

	// Check whether at least one (1) input file was provided

	
	if(argc < 2)
	{
		displayError(MISSING_COMMAND_LINE_ARGUMENTS, argv[0]);
		exit(-1);
	}
	
	//argv[1]

	symbol* symbols[SYMBOL_TABLE_SIZE] = { NULL };

	performPass1(symbols, argv[1], &addresses);
	
	// displaySymbolTable(symbols);

	// Display the assembly summary data
	// printf("\nStarting Address: 0x%X\nEnding Address: 0x%X\nProgram Size (bytes): %d\n", addresses.start, addresses.current, addresses.current - addresses.start);

	performPass2(symbols, argv[1], &addresses);

	printf("\n\nDone!\n\n");
}

// Determines the Format 3/4 flags and computes address displacement for Format 3 instruction
int computeFlagsAndAddress(symbol* symbolArray[], address* addresses, segment* segments, int format)
{
	int retAddress;
	char* buffer = malloc(strlen(segments->operand) + 1);
	strcpy(buffer, segments->operand);
	//char* buffer = segments->operand;
	int bitFlag = 0;

	if (buffer[0] == IMMEDIATE_CHARACTER) {
		bitFlag += FLAG_I;
		buffer = &buffer[1];
	}
	else if (buffer[0] == INDIRECT_CHARACTER) {
		bitFlag += FLAG_N;
		buffer = &buffer[1];
	}
	else {
		bitFlag += FLAG_N + FLAG_I;
		
		if (strstr(buffer, INDEX_STRING) != NULL) {
			int count = 0;
			while (buffer[count] != 'X') {
				count++;
			}
			bitFlag += FLAG_X;
			buffer[count - 1] = '\0';
		}
	}
	if (format == 4) {
		bitFlag += FLAG_E;
	}

	if (strcmp(segments->operation, "RSUB") == 0) {
		bitFlag *= FORMAT_3_MULTIPLIER;
		return bitFlag;
	}

	char* endptr;
	int opNum = strtol(buffer, &endptr, 10);
	if (*endptr == '\0') {
		if (format == 3) {
			bitFlag *= FORMAT_3_MULTIPLIER;
		}
		else {
			bitFlag *= FORMAT_4_MULTIPLIER;
		}
		bitFlag += opNum;
		return bitFlag;
	}
	else {
		retAddress = getSymbolAddress(symbolArray, buffer);
	}

	if (format == 4) {
		bitFlag *= FORMAT_4_MULTIPLIER;
		bitFlag += retAddress;
		return bitFlag;
	}

	//Compute PC-relative address
	int sum = addresses->current + addresses->increment;
	int displacement = retAddress - sum;
	
	if (displacement >= PC_MIN_RANGE && displacement <= PC_MAX_RANGE) { 
		bitFlag += FLAG_P;
	}
	else if (addresses->base != 0){
		displacement = retAddress - addresses->base;
		if (displacement >= 0 && displacement <= BASE_MAX_RANGE) {
			bitFlag += FLAG_B;
		}
		else {
			displayError(ADDRESS_OUT_OF_RANGE, segments->operation);
			exit(0);
		}
	}


	if (displacement < 0) {
		bitFlag++;
	}

	bitFlag *= FORMAT_3_MULTIPLIER;
	bitFlag += displacement;
	return bitFlag;
}

// Do no modify any part of this function
// Returns a new filename using the provided filename and extension
char* createFilename(char* filename, const char* extension)
{
	char* temp = (char*)malloc(sizeof(char) * strlen(filename) + 1);
	
	int n = strchr(filename, '.') - filename;
	strncpy(temp, filename, n);
	strcat(temp, extension);
	return temp;
}

// Do no modify any part of this function
// Writes existing data to Object Data file and resets values
void flushTextRecord(FILE* file, objectFileData* data, address* addresses)
{
	writeToObjFile(file, *data);
	data->recordAddress = addresses->current;
	data->recordByteCount = 0;
	data->recordEntryCount = 0;
}

// Returns a hex byte containing the registers listed in the provided operand
int getRegisters(char* operand)
{
	int regVal = getRegisterValue(operand[0]) * REGISTER_MULTIPLIER;
	if (getRegisterValue(operand[2]) != -1) {
		regVal = regVal + getRegisterValue(operand[2]);
	}
	return regVal;
}

// Do no modify any part of this function
// Returns the hex value for the provided register name
int getRegisterValue(char registerName)
{
	switch(registerName)
	{
		case 'A':
			return REGISTER_A;
		case 'B':
			return REGISTER_B;
		case 'L':
			return REGISTER_L;
		case 'S':
			return REGISTER_S;
		case 'T':
			return REGISTER_T;
		case 'X':
			return REGISTER_X;
		default:
			return -1;
	}
}

// Do no modify any part of this function
// Returns true if the provided string contains a numeric value; otherwise, false
bool isNumeric(char* string)
{
	for(int x = 0; x < strlen(string); x++)
	{
		if(!isdigit(string[x]))
		{
			return false;
		}
	}
	return true;
}

// Do no modify any part of this function
// Performs Pass 1 of the SIC/XE assembler
void performPass1(symbol* symbolTable[], char* filename, address* addresses)
{
	char line[INPUT_BUF_SIZE];
	FILE* file;
	int directiveType = 0;
	
	file = fopen(filename, "r");
	if (!file)
	{
		displayError(FILE_NOT_FOUND, filename);
		exit(-1);
	}

	while (fgets(line, INPUT_BUF_SIZE, file))
	{
		if (addresses->current >= 0x100000)
		{
			char value[10];
			sprintf(value, "0x%X", addresses->current);
			displayError(OUT_OF_MEMORY, value);
			exit(-1);
		}
		if (line[0] < 32) // Line is blank
		{
			displayError(BLANK_RECORD, NULL);
			exit(-1);
		}
		else if (line[0] == COMMENT) // Line is comment
		{
			continue;
		}
		else
		{
			segment* segments = prepareSegments(line);
						
			if (isDirective(segments->label) || isOpcode(segments->label))
			{
				displayError(ILLEGAL_SYMBOL, segments->label);
				exit(-1);
			}
			if (directiveType = isDirective(segments->operation))
			{
				if (isStartDirective(directiveType))
				{
					addresses->start = addresses->current = strtol(segments->operand, NULL, 16);
					continue;
				}
				else
				{
					addresses->increment = getMemoryAmount(directiveType, segments->operand);
				}
			}
			else if (isOpcode(segments->operation))
			{
				addresses->increment = getOpcodeFormat(segments->operation);
			}
			else
			{
				displayError(ILLEGAL_OPCODE_DIRECTIVE, segments->operation);
				exit(-1);
			}
			if (strlen(segments->label) > 0)
			{
				insertSymbol(symbolTable, segments->label, addresses->current);
			}
			addresses->current += addresses->increment;
		}
		memset(line, '\0', INPUT_BUF_SIZE);
	}
	fclose(file);
}

// Performs Pass 2 of the SIC/XE assembler
void performPass2(struct symbol* symbolTable[], char* filename, address* addresses)
{
	// Do not modify any of the provided code
	objectFileData objectData = { 0, { 0x0 }, { "\0" }, 0, 0x0, 0, { 0 }, 0, '\0', 0x0 };
	
	char inData[INPUT_BUF_SIZE];
	char objData[OUTPUT_BUF_SIZE];

	FILE *fileIn, *fileLst, *fileObj;
	int directiveType = 0;
		
	char* lstFilename = createFilename(filename, ".lst");
	char* objFilename = createFilename(filename, ".obj");

	fileIn = fopen(filename, "r");
	if (!fileIn)
	{
		displayError(FILE_NOT_FOUND, filename);
		exit(-1);
	}
	fileLst = fopen(lstFilename, "w");
	fileObj = fopen(objFilename, "w");
	
	while (fgets(inData, INPUT_BUF_SIZE, fileIn))
	{ 
	// Do not modify any of the code provided above
	
	// Place your code for the performPass2 function here
		int asciiVal = inData[0];
		if (asciiVal == 35) {
			continue;
		}
		objectData.recordType = 'T';

		segment* segments = prepareSegments(inData);

		//Test for directive
		if (isDirective(segments->operation) != 0) {
			if (isStartDirective(isDirective(segments->operation))) {
				objectData.recordType = 'H';
				strcpy(objectData.programName, segments->label);
				objectData.startAddress = addresses->start;
				objectData.recordAddress = addresses->start;
				objectData.programSize = addresses->current - addresses->start;
				addresses->current = addresses->start;
				writeToObjFile(fileObj, objectData);
				writeToLstFile(fileLst, addresses->start, segments, BLANK_INSTRUCTION);
				continue;
			}

			else if (isBaseDirective(isDirective(segments->operation))) {
				addresses->base = getSymbolAddress(symbolTable, segments->operand);
				writeToLstFile(fileLst, addresses->current, segments, BLANK_INSTRUCTION);
				continue;
			}

			else if (isEndDirective(isDirective(segments->operation))) {
				if (objectData.recordByteCount > 0) {
					//figure out what file to put here???
					flushTextRecord(fileObj, &objectData, addresses);
				}
				objectData.recordType = 'E';
				writeToObjFile(fileObj, objectData);
				writeToLstFile(fileLst, addresses->current, segments, BLANK_INSTRUCTION);
			}

			else if (isReserveDirective(isDirective(segments->operation))) {
				if (objectData.recordByteCount > 0) {
					flushTextRecord(fileObj, &objectData, addresses);
				}
				writeToLstFile(fileLst, addresses->current, segments, BLANK_INSTRUCTION);
				addresses->increment = getMemoryAmount(isDirective(segments->operation), segments->operand);
				objectData.recordAddress += addresses->increment;
			}

			else if (isDataDirective(isDirective(segments->operation))) {
				addresses->increment = getMemoryAmount(isDirective(segments->operation), segments->operand);
				int countDiff = MAX_RECORD_BYTE_COUNT - addresses->increment;
				if (objectData.recordByteCount > countDiff) {
					flushTextRecord(fileObj, &objectData, addresses);
				}
				
				int byteVal = getByteValue(isDirective(segments->operation), segments->operand);
				//add record entry
				objectData.recordEntries[objectData.recordEntryCount].numBytes = addresses->increment;
				objectData.recordEntries[objectData.recordEntryCount].value = byteVal;
				objectData.recordEntryCount += 1;
				objectData.recordByteCount += addresses->increment;
				writeToLstFile(fileLst, addresses->current, segments, byteVal);
			}
		}
		else if (isOpcode(segments->operation)) {
			int opVar = getOpcodeValue(segments->operation);
			addresses->increment = getOpcodeFormat(segments->operation);
			if (addresses->increment == -1) {
				//illegal opcode format error
				displayError(ILLEGAL_OPCODE_FORMAT, segments->operation);
				exit(0);
			}

			//align opcode var
			if (addresses->increment == 2) {
				opVar *= OPCODE_MULTIPLIER;
			}
			else if (addresses->increment == 3) {
				opVar *= OPCODE_MULTIPLIER * OPCODE_MULTIPLIER;
			}
			else if (addresses->increment == 4) {
				opVar *= OPCODE_MULTIPLIER * OPCODE_MULTIPLIER * OPCODE_MULTIPLIER;
			}

			//prepare instruction
			if (addresses->increment == 2) {
				opVar += getRegisters(segments->operand);
			}
			else if (addresses->increment == 3 || addresses->increment == 4) {
				opVar += computeFlagsAndAddress(symbolTable, addresses, segments, addresses->increment);
			}

			int countDiff = MAX_RECORD_BYTE_COUNT - addresses->increment;
			if (objectData.recordByteCount > countDiff) {
				flushTextRecord(fileObj, &objectData, addresses);
			}

			objectData.recordEntries[objectData.recordEntryCount].numBytes = addresses->increment;
			objectData.recordEntries[objectData.recordEntryCount].value = opVar;
			objectData.recordEntryCount += 1;
			objectData.recordByteCount += addresses->increment;
			writeToLstFile(fileLst, addresses->current, segments, opVar);
		}
		addresses->current += addresses->increment;
		memset(inData, 0, INPUT_BUF_SIZE);

	// Do not modify any of the code provided below
	}
	fclose(fileIn);
	fclose(fileLst);
	fclose(fileObj);
}

// Do no modify any part of this function
// Separates a SIC/XE instruction into individual sections
segment* prepareSegments(char* statement)
{
	segment* temp = malloc(sizeof(segment));
	strncpy(temp->label, statement, SEGMENT_SIZE - 1);
	strncpy(temp->operation, statement + SEGMENT_SIZE - 1, SEGMENT_SIZE - 1);
	strncpy(temp->operand, statement + (SEGMENT_SIZE - 1) * 2, SEGMENT_SIZE - 1);

	trim(temp->label); // Label
	trim(temp->operation); // Operation
	trim(temp->operand); // Operand
	return temp;
}

// Do no modify any part of this function
// Removes spaces from the end of a segment value
void trim(char value[])
{
	for (int x = 0; x < SEGMENT_SIZE; x++)
	{
		if (value[x] == SPACE)
		{
			value[x] = '\0';
		}
	}
}

// Do no modify any part of this function
// Write SIC/XE instructions along with address and object code information of source code listing file
void writeToLstFile(FILE* file, int address, segment* segments, int opcode)
{
	char ctrlString[27];
	int length;
	
	int directiveType = isDirective(segments->operation);
	if (isStartDirective(directiveType) || 
		isBaseDirective(directiveType) || 
		isReserveDirective(directiveType))
	{
		fprintf(file, "%-8X%-8s%-8s%-8s\n", address, segments->label, segments->operation, segments->operand);
	}
	else if (isEndDirective(directiveType))
	{
		fprintf(file, "%-8X%-8s%-8s%-8s", address, segments->label, segments->operation, segments->operand);
	}
	else
	{
		if (isDataDirective(directiveType))
		{
			length = getMemoryAmount(directiveType, segments->operand) * 2;
		}
		else
		{
			length = getOpcodeFormat(segments->operation) * 2;
		}
		sprintf(ctrlString, "%%-8X%%-8s%%-8s%%-8s    %%0%dX\n", length);

		fprintf(file, ctrlString, address, segments->label, segments->operation, segments->operand, opcode);
	}
}

// Do no modify any part of this function
// Write object code data to object code file
void writeToObjFile(FILE* file, objectFileData data)
{
	char ctrlString[27];
	
	if (data.recordType == 'H')
	{
		fprintf(file, "H%-6s%06X%06X\n", data.programName, data.startAddress, data.programSize);
	}
	else if (data.recordType == 'T')
	{
		fprintf(file, "T%06X%02X", data.recordAddress, data.recordByteCount);
		for (int x = 0; x < data.recordEntryCount; x++)
		{
			sprintf(ctrlString, "%%0%dX", data.recordEntries[x].numBytes * 2);
			fprintf(file, ctrlString, data.recordEntries[x].value);
		}
		fprintf(file, "\n");
	}
	else if (data.recordType == 'E')
	{
		fprintf(file, "E%06X", data.startAddress);
	}
	else if (data.recordType == 'M')
	{
		for (int x = 0; x < data.modificationCount; x++)
		{
			fprintf(file, "M%06X05+%s\n", data.modificationEntries[x], data.programName);
		}
	}
}