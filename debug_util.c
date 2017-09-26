//
// Created by Jake Lewis on 26/09/2017.
//

#include "debug_util.h"

bool DEBUG = false;

//Only prints if the global DEBUG flag has been set to true
//Replicates code from printf()
int printDebug(const char *format, ...)
{
    int done = 0;
    if (DEBUG)
    {
        va_list arg;

        va_start (arg, format);
        done = vfprintf (stdout, format, arg);
        va_end (arg);
    }

    return done;
}