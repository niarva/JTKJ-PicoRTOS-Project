#ifndef TEMPLATE_H
#define TEMPLATE_H

/**
 * @brief Interrupt handler function for button presses. Calls toggle_state_fxn() and sets button1_pressed and button2_pressed flags.
 */
void btn_fxn(uint gpio, uint32_t eventMask);

/**
 * @brief Toggles READ_IMU state. Called by btn_fxn.
 */
void toggle_state_fxn();

/**
 * @brief FreeRTOS task for handling IDLE state.
 * Calls imu_fn() when state changes to READ_IMU.
 */
void idle_task(void *arg);

/**
 * @brief Creates FreeRTOS tasks and initializes HAT components. Creates interrupts for button presses. Starts scheduler.
 * @return Never.
 */
int main();

enum state {
    IDLE=1, 
    RECV_MSG=2, 
    DISPLAY=3, 
    READ_IMU=4
};

#endif