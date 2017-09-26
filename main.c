#include <stdio.h>
#include <string.h>
#include "debug_util.c"

void evaluate(char*, int, int, char*);
const int MAX_STRING_LEN = 16;
const bool LOCAL_DEBUG_FLAG = true;

int main()
{
    DEBUG = LOCAL_DEBUG_FLAG;
    int i;
    char output[MAX_STRING_LEN];

    printDebug("Initial Loop Start\n");

    for (i = 1; i <= 100; i++)
    {
        printDebug("Setting initial output to null\n");
        /* Set first value in array to null termination,
         * effectively clearing the array,
         * if it is only read as a sequential string */
        output[0] = '\0';

        evaluate(output, 3, i, "Fizz");
        evaluate(output, 3, i, "Fizz");
        evaluate(output, 3, i, "Fizz");
        strcat(output, '\0');

        printf(output + '\n');
    }
}

void evaluate(char *buf, int factor, int value, char *result)
{
    printDebug("Evaluating || Factor: %d, Value: %d, Result: %s\n", factor, value, result);
    char *temp = (value % factor == 0) ? result : "";

    printDebug("Evaluating complete || Result: %s\n", temp);
    strcat(temp, buf);
}