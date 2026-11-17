#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * @brief FreeRTOS task for printing messages word by word to LCD-display when state is RECV_MSG.
 */
void display_task(void *args);
#endif