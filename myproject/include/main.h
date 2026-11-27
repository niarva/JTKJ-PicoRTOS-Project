#ifndef TEMPLATE_H
#define TEMPLATE_H

/**
 * @brief Interrupt handler function for button presses. Calls toggle_state_fxn() and sets button1_pressed and button2_pressed flags.
 */
void btn_fxn(uint gpio, uint32_t eventMask);


/**
 * @brief Creates FreeRTOS tasks and initializes HAT components. Creates interrupts for button presses. Starts scheduler.
 * @return Never.
 */
int main();

#endif