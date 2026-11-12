
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

//Add here necessary states
enum state { IDLE=1, MSG_READY=2 };
enum state programState = IDLE;

char message[3] = {0, 0, 0};
uint8_t curr_char = 0;

void print_fn(char *arg);

static volatile uint8_t button_pressed, debug_pressed;
static void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        button_pressed = true;
    }
    else if (gpio == BUTTON2) {
        debug_pressed = true;
    }
    toggle_led();
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
        /*int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        printf ("Enable gyro: %d\n",_enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf ("Gyro return:  %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf ("Accel return:  %d\n", _accel);*/
    } else {
        printf("Failed to initialize ICM-42670P.\n");
    }
    // Start collection data here. Infinite loop. 
    while (1)
    {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            if (curr_char >= 2) {
                curr_char = 0;
                print_fn(message);
            }
            if (ax > 0.95 && ax < 1.05) {
                message[curr_char++] = '-';
                
            } else if (ay > 0.95 && ay < 1.05) {
                message[curr_char++] = '.';
            }
            // printf("Accel: X=%f, Y=%f, Z=%f | Gyro: X=%f, Y=%f, Z=%f| Temp: %2.2f°C\n", ax, ay, az, gx, gy, gz, t);

        } else {
            printf("Failed to read imu data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
    }

}

void print_fn(char *arg) {
    for (int i = 0; arg[i] == '.' || arg[i] == '-'; i++) {
        printf("%c", arg[i]);
    }
}

static void print_task(void *arg){
    (void)arg;
    
    while(1){

        if (button_pressed) {
            print_fn(message);
            printf("  ");
            button_pressed = false;
            curr_char = 0;
        }
        if (debug_pressed) {
            for (int i = 0; message[i] == '.' || message[i] == '-'; i++) {
                printf("%c", message[i]);
                message[i] = 0;
            }
            printf("  ");
            printf("\n");
            debug_pressed = false;
            curr_char = 0;
            
        }



        /*if (button_pressed) {
             for (int i = 0; hellotext[i] != NULL; i++) {
                printf("%s", hellotext[i]);
            }
            button_pressed = false;
        }
        if (debug_pressed){
            for (int i = 0; hellotext_debug[i] != NULL; i++) {
                printf("%s", hellotext_debug[i]);
            }
            debug_pressed = false;
        }*/

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

    gpio_init(BUZZER_PIN);

    TaskHandle_t imuTask, hPrintTask, hReceiveTask;
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

    result = xTaskCreate(print_task,  // (en) Task function
            "print",              // (en) Name of the task 
            DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
            NULL,                 // (en) Arguments of the task 
            2,                    // (en) Priority of this task
            &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

