#include "display.h"

#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <queue.h>

#include "tkjhat/sdk.h"
#include "template.h"
#include "receive.h"

extern char line[INPUT_BUFFER_SIZE];
extern char msg[INPUT_BUFFER_SIZE];
extern volatile enum state programState;

void display_task(void *args) {

    while (true) {
        
        // RECV_MSG set by receive_task().
        if (programState == RECV_MSG) {
            buzzer_play_tone(650, 100);
            
            // Take first word of msg.
            char* word = strtok(msg, " ");
            while (word != NULL) {
                clear_display();
                char buf[10];

                // Split word if can't fit on screen at once.
                while (strlen(word) > 10) {
                    strncpy(buf, word, 8);
                    buf[8] = '-';
                    word = word + 8;
                    write_text(buf);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    clear_display();
                }

                clear_display();
                write_text(word);
                word = strtok(NULL, " "); // Next word.
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            clear_display();
            
            // Clear arrays.
            for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
                msg[i] = 0;
                line[i] = 0;
            }
            programState = IDLE;
            
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}