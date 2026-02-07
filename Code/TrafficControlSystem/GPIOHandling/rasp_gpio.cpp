#include "../GPIOHandling/rasp_gpio.hpp"
#include <gpiod.h>
#include <stdio.h>

#define DEFAULT_CHIP "gpiochip0"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lines[128] = {0};

int set_output_mode(const int line_offset) {
    if (!chip) chip = gpiod_chip_open_by_name(DEFAULT_CHIP);
    if (!chip) return -1;

    if (!lines[line_offset]) {
        lines[line_offset] = gpiod_chip_get_line(chip, line_offset);
        if (!lines[line_offset]) return -2;

        if (gpiod_line_request_output(lines[line_offset], "rasp_gpio", 0) < 0)
            return -3;
    }
    return 0;
}


int set_input_mode(const int line_offset) {
    if (!chip) chip = gpiod_chip_open_by_name(DEFAULT_CHIP);
    if (!chip) return -1;

    if (!lines[line_offset]) {
        lines[line_offset] = gpiod_chip_get_line(chip, line_offset);
        if (!lines[line_offset]) return -2;

        if (gpiod_line_request_input(lines[line_offset], "rasp_gpio") < 0)
            return -3;
    }
    return 0;
}

int rasp_gpio_set(const int line) {
    return gpiod_line_set_value(lines[line], 1);
}

int rasp_gpio_clear(const int line) {
    return gpiod_line_set_value(lines[line], 0);
}

void rasp_gpio_release(const int line) {
    return gpiod_line_release(lines[line]);
}

int rasp_gpio_read(const int line) {
    return gpiod_line_get_value(lines[line]);
}

/*
 * Uses pollcb -> blocks the thread => 0% CPU consumption
 */
int rasp_gpio_reqInt(const int line_offset, void* data, int(*callback)(int, unsigned int, const struct timespec*, void*))
{
    const int ret = gpiod_ctxless_event_monitor(
        DEFAULT_CHIP,
        GPIOD_CTXLESS_EVENT_CB_RISING_EDGE,
        line_offset,
        false,                                  // active state => true if high
        "debouncing_function",
        nullptr,                        /*  man ppoll: If tmo_p is specified as NULL, then ppoll() can block indefinitely */
        nullptr,
        callback,
        data);

    return ret;
}