#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "morse.h"
#include "template.h"

#include "receive.h"

char line[INPUT_BUFFER_SIZE];
char msg[INPUT_BUFFER_SIZE];
size_t parse_index = 0;

extern volatile enum state programState;

uint8_t parse_input() {
    
    int c = 0;
    while (c != PICO_ERROR_TIMEOUT) {
        c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            break;
        }

        if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
        if (c == '\n'){
            // terminate and process the collected line
            line[parse_index] = '\0'; 
            parse_index = 0;

            return true;
        }
        else if (parse_index < INPUT_BUFFER_SIZE - 1){
            printf("%c", (char)c);
            if (parse_index == 0) {
                write_text("Receiving");
            }
            line[parse_index++] = (char)c;

        }
        else { //Overflow: print and restart the buffer with the new character. 
            line[INPUT_BUFFER_SIZE - 1] = '\0';
            // printf("Overflow __[RX]:\"%s\"__\n", line);
            parse_index = 0; 
            line[parse_index++] = (char)c; 
        }
    }
    return false;

}

void receive_task(void *arg) {
    (void)arg;
    
    while (1){
        size_t big_array_size = 0;
        
        if (parse_input()) {

            // Calculate number of words in message.
            buzzer_play_tone(550, 50);
            for (int i = 1; i < sizeof(line) / sizeof(line[0]); i++){
                if (line[i] == ' ' && line[i - 1] == ' '){
                    big_array_size++;
                }
            }

            buzzer_play_tone(450, 50);
            char big_array[big_array_size][INPUT_BUFFER_SIZE];
            size_t big_array_index = 0;
            int line_size = sizeof(line) / sizeof(line[0]);
            int last_word = 0;

            // Add each morse word as a separate element into big_array[].
            for (int i = 1; i < line_size; i++){
                if (line[i] == ' ' && line[i - 1] == ' '){
                    char * word = strncpy(big_array[big_array_index++], line + last_word, i - last_word);
                    word[i - last_word] = '\0';
                    big_array[big_array_index - 1][i - last_word] = '\0';

                    last_word = i + 1; 
                }
            }

            buzzer_play_tone(350, 50);
            int line_index = 0;
            
            // Split each word into characters and translate characters into latin alphabet.
            for (int i = 0; i < big_array_size; i++){

                char *letter = strtok(big_array[i], " ");
                while (letter != NULL) {
                    char m = morse_to_char(letter);
                    if (m != 0) {
                        msg[line_index++] = m;
                    }
                    letter = strtok(NULL, " ");
                }

                msg[line_index++] = ' ';

            }

            buzzer_play_tone(650, 50);
            clear_display();
            programState = RECV_MSG;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
    }

}