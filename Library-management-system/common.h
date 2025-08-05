#ifndef COMMON_H
#define COMMON_H

#include "types.h"

// Correct and singular definition of the struct
typedef struct sign_in
{
    char name[MAX_NAME_LEN];
    char pass[MAX_PASSWD_LEN];
} sign_in_t;

// You may also have other common structs here, but only define each one once.

#endif