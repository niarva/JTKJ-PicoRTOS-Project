#ifndef RECEIVE_H
#define RECEIVE_H

// Maximum size of data received from serial in bytes
#define INPUT_BUFFER_SIZE 256

/**
 * @brief Receives characters via USB and adds them to line[] until timeout or newline.
    Returns false if timeout occurs and true if message was fully received.
*/
uint8_t parse_input();


/**
 * @brief FreeRTOS task for receiving morse code via USB and parsing the message into a string of latin alphabet.
 * Final message is stored in msg[]
 */
void receive_task(void *arg);
#endif