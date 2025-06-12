#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cJSON.h"
#include "cmd.h"

/*
configure the command line interface for the application.
json template:
{
    "valve_count": 5,
}

*/