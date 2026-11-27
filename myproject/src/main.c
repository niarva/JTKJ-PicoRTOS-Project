
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "main.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 
#define MSG_MAX_LEN 512

volatile uint8_t button1_pressed, button2_pressed;
volatile uint8_t space_pressed;
volatile uint8_t send_msg = false;
char msg[MSG_MAX_LEN];
size_t msg_index = 0;


void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        button1_pressed = true;
    }
    else if (gpio == BUTTON2) {
        button2_pressed = true;
    }
}

void msg_push_char(char c) {
    if (msg_index < MSG_MAX_LEN) {
        msg[msg_index] = char;
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
                    msg_push_char('-');
                    msg_index++;
                    buzzer_play_tone(440, 150);
                } else if (ay > 0.90 && ay < 1.10) {
                    msg_push_char('.');
                    msg_index++;
                    buzzer_play_tone(440, 150);
                }
                buzzer_play_tone(440, 150);
                
                button1_pressed = false;
            } else if (button2_pressed) {
                if (space_pressed) {
                    send_msg = true;
                    space_pressed = false;
                    buzzer_play_tone(550, 250);
                } else {
                    msg_push_char(' ');
                    msg_index++;
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

void send_task() {
    while (true) {
        if (send_msg) {
            for (size_t i = 0; i < strlen(msg); i++) {
                printf("%c ", msg[i]);
            }
            printf("  \n");
            send_msg = false;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
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

    TaskHandle_t imuTaskHandle sendTaskHandle;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(imu_task,       // (en) Task function
                "IMU",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &imuTaskHandle);    // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("IMU Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(send_task,   // (en) Task function
            "send",                   // (en) Name of the task 
            1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
            NULL,                    // (en) Arguments of the task 
            1,                       // (en) Priority of this task
            &sendTaskHandle);     // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Display Task created failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    printf("  \n");
    vTaskStartScheduler();


    // Never reach this line.
    return 0;
}

