#include <string.h>
#include "debug_util.c"

void evaluate(char*, int, int, char*);
const int maxStringLengthConst = 17;
const int maxWordLengthConst = 7;
const int maxDigitLengthConst = 4;

int main(void)
{
    int i;
    char output[maxStringLengthConst];
    char intVal[maxDigitLengthConst];

    printDebug("Initial Loop Start\n");

    for (i = 1; i <= 100; i++)
    {
        printDebug("Setting initial output to null\n\n");
        /* Set first value in array to null termination,
         * effectively clearing the array,
         * if it is only read as a sequential string */
        output[0] = '\0';

        snprintf(intVal, maxDigitLengthConst,"%d",i);

        evaluate(output, 3, i, "Fizz");
        evaluate(output, 4, i, "Buzz");
        evaluate(output, 5, i, "Sizzle");

        output[sizeof(output)] = '\0';

        if (output[0] == '\0')
        {
            strncat(output, intVal, maxDigitLengthConst);
        }

        printf("%s\n", output);
    }

    //Only optional in C99
    return 0;
}

void evaluate(char *buf, int factor, int value, char *result)
{
    printDebug("Evaluating || Factor: %d, Value: %d, Success Result: %s\n", factor, value, result);
    char temp[maxWordLengthConst];

    strncpy(temp, ((value % factor == 0) ? result : ""), maxWordLengthConst -1);

    printDebug("Evaluating complete || Result: \"%s\"\n\n", temp);

    //Check array is not empty
    if (value % factor == 0)
    {
        strncat(buf, temp, maxStringLengthConst -1);
    }
}