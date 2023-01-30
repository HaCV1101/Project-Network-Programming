/**
 * General utilities
 */
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Clear stdin buffer
 */
void clear_stdin_buff()
{
    char ch;
    while ((ch = getchar()) != EOF && ch != '\n')
        ;
}

void ranCapcha(char *capcha)
{
    srand(time(NULL));
    int check = 0;
    int x;

    for (int i = 0; i < 6; i++)
    {
        if (i == 5)
        {
            if (check == 5)
                capcha[i] = rand() % 26 + 'A';
            else
                capcha[i] = rand() % 10 + '0';
            break;
        }
        x = rand() % 2;
        if (x)
        {
            check++;
            capcha[i] = rand() % 10 + '0';
        }
        else
        {
            check += 2;
            capcha[i] = rand() % 26 + 'A';
        }
    }
}