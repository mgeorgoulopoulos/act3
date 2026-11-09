// log.cpp

#include <stdio.h>

//#define FINAL_VERSION

// FUNCTIONS ///////////////////////////////////////////////

void OpenLog()
{
#ifndef FINAL_VERSION

	FILE* log_file = fopen("log.htm" , "w+b");
	fprintf(log_file , "<html><head><title>Program Log</title></head><i><h1>Program Log</h1></i><hr>");
	fclose(log_file);
#endif`
}

void CloseLog()
{
#ifndef FINAL_VERSION
	FILE* log_file = fopen("log.htm","a+");
	fprintf(log_file , "<hr><p align =right><b><i>Reversed Engineers - All rights Reversed</i></b></p></body></html>");
	fclose(log_file);
#endif
}

void WriteToLog(const char* text )
{
#ifndef FINAL_VERSION

	FILE* log_file = fopen("log.htm","a+");

	fprintf(log_file , text );
	fprintf(log_file , "<br>");

	fclose(log_file);
#endif
}