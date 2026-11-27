
#include <stdio.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>

#include "tkjhat/sdk.h"
#include "imu.h"
#include "template.h"

// Threshold for registering input characters.
const int GYRO_THRESHOLD = 200;

extern volatile enum state programState;
extern volatile uint8_t button1_pressed, button2_pressed, space_pressed;

float avg(float *arr, int size) {
    float result = 0;
    for (int i = 0; i < size; i++) {
        result += arr[i];
    }
    return (result / size);
}

void imu_fn() {
    
    static float ax, ay, az, gx, gy, gz, t;

    // Arrays for storing samples of gyroscope values.
    float gz_values[200];
    float gx_values[200];
    int counter = 0;

    write_text("Reading...");

    // READ_IMU toggled by button_fxn.
    while (programState == READ_IMU) {
        while (counter < 200) {
            if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {

                // Print space if left button pressed. Send message with two presses.
                if (button2_pressed) {
                    if (space_pressed) {
                        printf(" \n");
                        programState = IDLE;
                        space_pressed = false;
                        buzzer_play_tone(550, 250);
                    } else {
                        printf(" ");
                        space_pressed = true;
                        buzzer_play_tone(440, 150);
                    }
                    button2_pressed = false;

                } else {
                    // Add values into corresponding arrays.
                    gz_values[counter] = gz;
                    gx_values[counter] = gx;
                    counter++;
                }

            } else {
                printf("Failed to read imu data\n");
            }
            
        }

        float gz_avg = avg(gz_values, 200);
        float gx_avg = avg(gx_values, 200);

        // Print dot or dash if threshold surpassed.
        if (ABS(gx_avg) > GYRO_THRESHOLD) {
            printf(".");
            buzzer_play_tone(1000, 250);
            space_pressed = false;
        } else if (ABS(gz_avg) > GYRO_THRESHOLD) {
            printf("-");
            buzzer_play_tone(1000, 500);
            space_pressed = false;
        }

        // Clear arrays and counter.
        counter = 0;
        for (int i = 0; i < 200; i++) {
            gz_values[i] = 0;
            gx_values[i] = 0;
        }
    }

    clear_display();
    write_text("Sent!");
    clear_display();

}
