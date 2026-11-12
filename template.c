
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 
#define INPUT_BUFFER_SIZE 64

//Add here necessary states
enum state { IDLE=1, RECV_MSG=2, };
enum state programState = IDLE;


char line[INPUT_BUFFER_SIZE];
char next_char;

static volatile uint8_t button1_pressed, button2_pressed, recv_msg;
static volatile uint8_t space_pressed;



static void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        button1_pressed = true;
    }
    else if (gpio == BUTTON2) {
        button2_pressed = true;
    }
    toggle_led();
}

static void receive_task(void *arg){
    (void)arg;
    
    // size_t index = 0;
    
    while (1){
        //OPTION 1
        // Using getchar_timeout_us https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#group_pico_stdio_1ga5d24f1a711eba3e0084b6310f6478c1a
        // take one char per time and store it in line array, until reeceived the \n
        // The application should instead play a sound, or blink a LED. 
        /*int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT){// I have received a character
            if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
            if (c == '\n'){
                // terminate and process the collected line
                line[index] = '\0'; 
                printf("__[RX]:\"%s\"__\n", line); //Print as debug in the output
                index = 0;
                vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
            }
            else if(index < INPUT_BUFFER_SIZE - 1){
                line[index++] = (char)c;
            }
            else { //Overflow: print and restart the buffer with the new character. 
                line[INPUT_BUFFER_SIZE - 1] = '\0';
                printf("__[RX]:\"%s\"__\n", line);
                index = 0; 
                line[index++] = (char)c; 
            }
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }*/
        //OPTION 2. Use the whole buffer. 
        absolute_time_t next = delayed_by_us(get_absolute_time(), 500);//Wait 500 us
        int read = stdio_get_until(line,INPUT_BUFFER_SIZE,next);
        if (read == PICO_ERROR_TIMEOUT){
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }
        else {
            line[read] = '\0'; //Last character is 0
            printf("__[RX] \"%s\"\n__", line);
            //programState = RECV_MSG;
            recv_msg = true;
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }


}


void imu_task(void *pvParameters) {
    (void)pvParameters;

    
    float ax, ay, az, gx, gy, gz, t;
    // Setting up the sensor. 
    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("ICM-42670P could not initialize accelerometer or gyroscope");
        }
    } else {
        printf("Failed to initialize ICM-42670P.\n");
    }

    while (1)
    {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            if (button1_pressed) {
                if (ax > 0.90 && ax < 1.1) {
                    printf("-");
                } else if (ay > 0.90 && ay < 1.10) {
                    printf(".");
                }
                buzzer_play_tone(440, 150);
                button1_pressed = false;
            } else if (button2_pressed) {
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
            }

        } else {
            printf("Failed to read imu data\n");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

}

static void display_task(void *args) {

    init_display();
    clear_display();


    while (true) {
        if (recv_msg == true) {
            clear_display();
            write_text(line);
            // programState = IDLE;
            recv_msg = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
    }
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

    TaskHandle_t imuTask, displayTaskHandle, hReceiveTask;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(imu_task,       // (en) Task function
                "IMU",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &imuTask);    // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("IMU Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(display_task,   // (en) Task function
            "display",                   // (en) Name of the task 
            1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
            NULL,                    // (en) Arguments of the task 
            2,                       // (en) Priority of this task
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

