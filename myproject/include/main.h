#ifndef TEMPLATE_H
#define TEMPLATE_H

/**
 * @brief Interrupt handler function for button presses. Sets button1_pressed and button2_pressed flags.
 */
void btn_fxn(uint gpio, uint32_t eventMask);

/**
 * @brief Task for sending messages once send_msg = true
 */
void send_task();

/**
 * @brief Helper for pushing next symbol to msg.
 */
void msg_push_char(char c);

/**
 * @brief FreeRTOS task for reading imu position. Calls msg_push_char and sets send_msg flag.
 */
void imu_task(void *pvParameters);

/**
 * @brief Creates FreeRTOS tasks and initializes HAT components. Creates interrupts for button presses. Starts scheduler.
 * @return Never.
 */
int main();

#endif
