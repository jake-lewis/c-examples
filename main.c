#include <string.h>
#include "debug_util.c"

void evaluate(char*, int, int, char*);
const int MAX_STRING_LEN = 17;
const int MAX_WORD_LEN = 7;
const int MAX_DIGIT_COUNT_LEN = 4;
const bool LOCAL_DEBUG_FLAG = true;

int main()
{
    DEBUG = LOCAL_DEBUG_FLAG;
    int i;
    char output[MAX_STRING_LEN];
    char intVal[MAX_DIGIT_COUNT_LEN];

    printDebug("Initial Loop Start\n");

    for (i = 1; i <= 100; i++)
    {
        printDebug("Setting initial output to null\n");
        /* Set first value in array to null termination,
         * effectively clearing the array,
         * if it is only read as a sequential string */
        output[0] = '\0';

        snprintf(intVal, MAX_DIGIT_COUNT_LEN,"%d",i);

        evaluate(output, 3, i, "Fizz");
        evaluate(output, 4, i, "Buzz");
        evaluate(output, 5, i, "Sizzle");
        strcat(output, '\0');

        char *result = (output == "") ? intVal : output;

        printf("%s\n", result);
    }
}

void evaluate(char *buf, int factor, int value, char *result)
{
    printDebug("Evaluating || Factor: %d, Value: %d, Result: %s\n", factor, value, result);
    char temp[MAX_WORD_LEN];
    strncpy(temp, ((value % factor == 0) ? result : ""), MAX_WORD_LEN -1);

    printDebug("Evaluating complete || Result: \"%s\"\n", temp);

    //Check array is not empty
    if (temp[0] != 0)
    {
        strcat(temp, buf);
    }
}