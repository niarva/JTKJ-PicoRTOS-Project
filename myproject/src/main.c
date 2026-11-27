/*
    Arvi Ainali
    Jaakko Harju
    Eevertti Jauhiainen
*/


#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "imu.h"
#include "main.h"
#include "receive.h"
#include "display.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

volatile enum state programState = IDLE;

volatile uint8_t button1_pressed, button2_pressed;
volatile uint8_t space_pressed;


void toggle_state_fxn() {
    if (programState == READ_IMU) {
        programState = IDLE;
    }
    else if (programState == IDLE) {
        programState = READ_IMU;
    }
}

void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        button1_pressed = true;
        toggle_state_fxn();
    }
    else if (gpio == BUTTON2) {
        button2_pressed = true;
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

    init_display();

    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("ICM-42670P could not initialize accelerometer or gyroscope");
        }
    } else {
        printf("Failed to initialize ICM-42670P.\n");
    }

    TaskHandle_t idleTaskHandle, displayTaskHandle, receiveTaskHandle;
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
            &receiveTaskHandle);         // (en) A handle to control the execution of this task

        if(result != pdPASS) {
        printf("Receive Task created failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    printf("  \n");
    vTaskStartScheduler();


    // Never reach this line.
    return 0;
}

