#ifndef IMU_H
#define IMU_H

// Macro for calculating absolute values.
#define ABS(x) (x > 0 ? x : -x)

/**
 * @brief Reads gyroscope values from IMU and prints dot or dash corresponding to the movement.
 */
void imu_fn();

/**
 * @brief Helper function for calculating average values.
 * @param arr Input array of floats.
 * @param size Number of elements in array.
 * @return Arithmetic mean of array elements as float.
 */
float avg(float *arr, int size);
#endif