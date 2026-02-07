
#ifndef TESTPIN_OUT_RASP_GPIO_HPP
#define TESTPIN_OUT_RASP_GPIO_HPP


/**
 * @brief Ensures that a GPIO line is initialized and ready for output.
 *
 * This function checks if the GPIO chip is open and the specified line
 * is configured. If not, it opens the chip (if needed), gets the line,
 * and requests it for output mode with initial value 0.
 *
 * @param line_offset The GPIO pin number (line offset) to initialize.
 * @return 0 on success,
 *        -1 if the chip couldn't be opened,
 *        -2 if the line couldn't be retrieved,
 *        -3 if the line couldn't be requested for output.
 */

int set_output_mode(int line_offset);

/**
 * @brief Ensures that a GPIO line is initialized and ready for input.
 *
 * This function checks if the GPIO chip is open and the specified line
 * is configured. If not, it opens the chip (if needed), gets the line,
 * and requests it for input mode.
 *
 * @param line_offset The GPIO pin number (line offset) to initialize.
 * @return 0 on success,
 *        -1 if the chip couldn't be opened,
 *        -2 if the line couldn't be retrieved,
 *        -3 if the line couldn't be requested for input.
 */
int set_input_mode(int line_offset);

/**
 * @brief Sets a GPIO pin to high (1).
 *
 * @param line The GPIO pin number.
 * @return 0 on success, negative value on failure.
 */
int rasp_gpio_set(int line);

/**
 * @brief Sets a GPIO pin to low (0).
 *
 * @param line The GPIO pin number.
 * @return 0 on success, negative value on failure.
 */
int rasp_gpio_clear(int line);

/**
 * @brief Release a previously reserved line.
 *
 * @param line The GPIO pin number.
 */
void rasp_gpio_release(int line);

/**
 * @brief Reads the current logical value of a GPIO pin.
 *
 * @param line The GPIO pin number.
 * @return 1 if high, 0 if low, or negative value on failure.
 */
int rasp_gpio_read(int line);

int rasp_gpio_reqInt(int line_offset, void* data,  int(*callback)(int, unsigned int, const struct timespec*, void*));

#endif //TESTPIN_OUT_RASP_GPIO_HPP