/*  PWM Linux Device Driver developed targeting RPi4 Model B
 * 
 *  All rights reserved to 
 *      - André Martins
 *      - Mariana Martins 
 */

#ifndef __LED_DD__
#define __LED_DD__

/* KERNEL INCLUDE SECTION */
#include <linux/types.h>

/* MACROS */
#define NUM_SELECT_REG  6
#define NUM_PWM_PINS 7

// BRCM2708 Physical addresses for RPi4 Model B
#define BASE_ADDR	    0xFE000000
#define GPIO_OFFSET	    0x00200000
#define PWM_OFFSET          0x0000C000  // PWM at 0xFE20C000
#define CLK_OFFSET          0x001010A0  // CLK registers start at 0xFE1010A0

#define CLK_CONFIG_ADDR    (BASE_ADDR + CLK_OFFSET)		// 0xFE1010A0
#define GPIO_BASE_ADDR     (BASE_ADDR + GPIO_OFFSET)		// 0xFE200000
#define PWM_CONFIG_ADDR    (GPIO_BASE_ADDR + PWM_OFFSET)	// 0xFE20C000

// Clock Manager Password - required for any configuration on CLK registers
#define CM_PASSWORD         0x5A000000

// CLOCK configurations
// Trying PLLD/4 = 500MHz/4 = 125 MHz (standard for RPi PWM)
// If this doesn't work, will fall back to 19.2 MHz oscillator
 #define CLK_PWM 187500000U  // PLLD/4 clock (125 MHz) - but experimentally I got 187.5MHz

/* TYPES DEFINITION */

typedef enum {
    INPUT,      // 000 = GPIO Pin XX is an input
    OUTPUT,     // 001 = GPIO Pin XX is an output
    ALT5,       // 010 = GPIO Pin XX takes alternate function 5
    ALT4,       // 011 = GPIO Pin XX takes alternate function 4
    ALT0,       // 100 = GPIO Pin XX takes alternate function 0
    ALT1,       // 101 = GPIO Pin XX takes alternate function 1
    ALT2,       // 110 = GPIO Pin XX takes alternate function 2
    ALT3,       // 111 = GPIO Pin XX takes alternate function 3    
} FSELx;

typedef enum {
    CHIP0,
    CHIP1
} PWMchips;

typedef enum {
    CH_0,      // in Datasheet lower channel is Channel 1; CH_1 = 0
    CH_1
} PWMchannels;

typedef enum {
    GPIO12,
    GPIO13,
    GPIO18,
    GPIO19,
    GPIO40,
    GPIO41,
    GPIO45,
} PWM_GPIO;

typedef struct {
    PWM_GPIO gpioPin;
    int pin;
    int FSELx;
    PWMchips chip;
    PWMchannels channel;
} gpioPWM_config;

typedef struct {
    u32 GPFSEL[NUM_SELECT_REG];
} GPIORegister;

typedef struct {
    u32 CTL;            // 0x00
    u32 STA;            // 0x04
    u32 DMAC;           // 0x08
    u32 _Reserved0;     // 0x0C
    u32 RNG1;           // 0x10
    u32 DAT1;           // 0x14
    u32 FIF1;           // 0x18
    u32 _Reserved1;     // 0x1C
    u32 RNG2;           // 0x20
    u32 DAT2;           // 0x24
} PWMRegister;

typedef struct {
    u32 CM_PWMCTL;  // 0xFE1010A0
    u32 CM_PWMDIV;  // 0xFE1010A4
} CLKRegister;

/* VARIABLES DECLARATION */
extern gpioPWM_config PWMconfig[];

/* Function declarations */
void SetGPIOFunction(GPIORegister* s_GPIORegister, 
                     PWM_GPIO GPIO_Pin, 
                     const FSELx functionCode,
                     CLKRegister* s_CLK);

void SetPWM(PWMRegister __iomem *regs,
            const PWM_GPIO gpio_id,
            int frequency,
            int duty_cycle,
            CLKRegister __iomem *clk_regs);

void init_clk(CLKRegister __iomem *clk);

#endif // __LED_DD__
