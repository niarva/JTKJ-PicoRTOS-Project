#include <string.h>
#include <stdio.h>
#include "morse.h"

const size_t MORSE_TABLE_SIZE = 36;

char morse_to_char(char *code) {
    for (int i = 0; i < MORSE_TABLE_SIZE; i++) {
        if (strcmp(MORSE_TABLE[i], code) == 0) {
            if (i < 26) {
                return 'A' + i; // Return letter at index i in the alphabet.
            }
            return '0' + (i - 26);
        }
    }
    printf("No corresponding character found.");
    return 0;
}

