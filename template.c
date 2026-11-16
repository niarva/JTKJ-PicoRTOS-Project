
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <timers.h>

#include "tkjhat/sdk.h"

#define ABS(x) (x > 0 ? x : -x)

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 
#define INPUT_BUFFER_SIZE 256

//Add here necessary states
enum state { IDLE=1, RECV_MSG=2, DISPLAY=3, READ_IMU=4};
volatile enum state programState = IDLE;

void imu_fn();

char line[INPUT_BUFFER_SIZE];
char msg[INPUT_BUFFER_SIZE];
char next_char;

static volatile uint8_t button1_pressed, button2_pressed;
static volatile uint8_t space_pressed;
size_t parse_index = 0;

static const char *MORSE_TABLE[36] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",
    ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",
    "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."
};


char morse_to_char(char *code) {
    for (int i = 0; i < 36; i++) {
        if (strcmp(MORSE_TABLE[i], code) == 0) {
            if (i < 26) {
                return 'A' + i;
            }
            return '0' + (i - 26);
        }
    }
}

void toggle_state_fxn() {
    if (programState == READ_IMU) {
        programState = IDLE;
    }
    else if (programState == IDLE) {
        programState = READ_IMU;
    }
}

static void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        button1_pressed = true;
        toggle_state_fxn();
    }
    else if (gpio == BUTTON2) {
        button2_pressed = true;
    }
    toggle_led();
}

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
            // printf("__[RX]:\"%s\"__\n", line); //Print as debug in the output
            parse_index = 0;

            return true;
        }
        else if(parse_index < INPUT_BUFFER_SIZE - 1){
            // printf("%c", (char)c);
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

static void receive_task(void *arg){
    (void)arg;
    
    
    while (1){
        size_t big_array_size = 0;
        if (parse_input()) {
            for (int i = 1; i < sizeof(line) / sizeof(line[0]); i++){
                if (line[i] == ' ' && line[i - 1] == ' '){
                    big_array_size++;
                }
            }

            char big_array[big_array_size][256];
            size_t big_array_index = 0;
            int line_size = sizeof(line) / sizeof(line[0]);
            int last_word = 0;

            for (int i = 1; i < line_size; i++){
                if (line[i] == ' ' && line[i - 1] == ' '){
                    char * word = strncpy(big_array[big_array_index++], line + last_word, i - last_word);
                    word[i - last_word] = '\0';
                    big_array[big_array_index - 1][i - last_word] = '\0';

                    last_word = i + 1; 
                }
            }



            int line_index = 0;
            

            for (int i = 0; i < big_array_size; i++){

                char *letter = strtok(big_array[i], " ");
                while (letter != NULL) {
                    char m = morse_to_char(letter);
                    msg[line_index++] = m;
                    letter = strtok(NULL, " ");
                }

                msg[line_index++] = ' ';

            }

            programState = RECV_MSG;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
    }



}

void idle_task(void *arg) {

    while (true) {
        while (programState == IDLE) {
            continue;
        }
        if (programState == READ_IMU) {
            imu_fn();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

float avg(float *arr, int size) {
    float result = 0;
    for (int i = 0; i < size; i++) {
        result += arr[i];
    }
    return (result / size);
}

void imu_fn() {
    
    static float ax, ay, az, gx, gy, gz, t;
    float gy_values[200];
    float gx_values[200];
    int counter = 0;

    while (programState == READ_IMU) {
        while (counter < 200) {
            if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {


                gy_values[counter] = gy;
                gx_values[counter] = gx;
                counter++;
                // printf("ax: %.2f, ay: %.2f\n", ax, ay);
                /*if (gx > 1.0) {
                    printf("-");
                    buzzer_play_tone(440, 150);
                } else if (gy > 1.5) {
                    printf(".");
                    buzzer_play_tone(440, 150);
                }*/
                
                button1_pressed = false;
            } /*else if (button2_pressed) {
                if (space_pressed) {
                    printf(" \n");
                    space_pressed = false;
                    buzzer_play_tone(550, 250);
                } else {
                    printf(" ");
                    space_pressed = true;
                    buzzer_play_tone(440, 150);
                }
                button2_pressed = false;
            }*/ else {
                printf("Failed to read imu data\n");
            }
            
        }

        float gy_avg = avg(gy_values, 200);
        float gx_avg = avg(gx_values, 200);

        //printf("gy: %.2f ", gy_avg);
        // printf("gx: %.2f\n", gx_avg);
        if (ABS(gy_avg) > 90) {
            printf("-");
            buzzer_play_tone(1000, 500);
        } else if (ABS(gx_avg) > 90) {
            printf(".");
            buzzer_play_tone(1000, 250);
        }

        counter = 0;
        for (int i = 0; i < 200; i++) {
            gy_values[i] = 0;
            gx_values[i] = 0;
        }
    }


}

static void display_task(void *args) {

    init_display();


    while (true) {
        if (programState == RECV_MSG) {
            buzzer_play_tone(650, 500);
            

            char* word = strtok(msg, " ");
            while (word != NULL) {
                printf("%s ", word);
                clear_display();
                write_text(word);
                word = strtok(NULL, " ");
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            

            for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
                msg[i] = 0;
                line[i] = 0;
            }
            programState = IDLE;
            
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.


    gpio_init(BUTTON1);
    gpio_init(BUTTON2);
    gpio_set_irq_enabled_with_callback(BUTTON1,  GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    gpio_set_irq_enabled_with_callback(BUTTON2,  GPIO_IRQ_EDGE_FALL, true, btn_fxn);

    init_buzzer();
    buzzer_play_tone(440, 500);

    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("ICM-42670P could not initialize accelerometer or gyroscope");
        }
    } else {
        printf("Failed to initialize ICM-42670P.\n");
    }

    TaskHandle_t idleTaskHandle, displayTaskHandle, hReceiveTask;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(idle_task,       // (en) Task function
                "IMU",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &idleTaskHandle);    // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("IMU Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(display_task,   // (en) Task function
            "display",                   // (en) Name of the task 
            1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
            NULL,                    // (en) Arguments of the task 
            1,                       // (en) Priority of this task
            &displayTaskHandle);     // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Display Task created failed\n");
        return 0;
    }

    result = xTaskCreate(receive_task,  // (en) Task function
            "receive",             // (en) Name of the task 
            DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
            NULL,                 // (en) Arguments of the task 
            2,                    // (en) Priority of this task
            &hReceiveTask);         // (en) A handle to control the execution of this task

        if(result != pdPASS) {
        printf("Receive Task created failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

