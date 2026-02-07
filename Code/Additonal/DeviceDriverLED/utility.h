#ifndef __LED_DD__
#define __LED_DD__

/* KERNEL INCLUDE SECTION */
#include <linux/types.h>    // for uint32_t

#define NUM_SELECT_REG   6
#define NUM_SET_CLR_REG  2


// BRCM2708 Physical GPIO_BASE_ADDR: 0x7E200000 is legacy address
#define GPIO_BASE_ADDR   0XFE200000

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

/*
    How to access register based on BRCM2708 datasheet memory layout
    What is needed:
        - Configuration register:GPFSELx
        - SET register: GPSETx
        - CLEAR register: GPCLRx
    Registers are 32 bits

    Reserved sections are destined for memory parts that won't be written,
    since this struct will be directly mapped into memory
 */
typedef struct{
    uint32_t GPFSEL[NUM_SELECT_REG];
    uint32_t Reserved0;
    uint32_t GPSET[NUM_SET_CLR_REG];
    uint32_t Reserved1;
    uint32_t GPCLR[NUM_SET_CLR_REG];
} GPIORegister; 

void SetGPIOFunction    (GPIORegister* s_GPIOregister, 
                        const int GPIO_PinNum, 
                        const FSELx functionCode);

void SetGPIOState   (GPIORegister* s_GPIOregister, 
                    const int GPIO_PinNum, 
                    const bool outputValue);


#endif // __LED_DD__