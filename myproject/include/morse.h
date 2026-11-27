#ifndef MORSE_H
#define MORSE H



static const char *MORSE_TABLE[36] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",
    ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",
    "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."
};


/**
 * @brief Translates morse character into latin alphabet A-Z and 0-9.
 * @param code Single morse code letter as string.
 * @return Corresponding alphanumeric character as char.
 * Returns 0 if no character is found.
 */
char morse_to_char(char *code);

#endif