/*  PWM Linux Device Driver developed targeting RPi4 Model B
 * 
 *  All rights reserved to 
 *      - André Martins
 *      - Mariana Martins 
 */

/* KERNEL INCLUDE SECTION */
#include <linux/kernel.h>
#include <linux/module.h>   // metadata macros
#include <linux/delay.h>    // usleep
/* NOTE: Using actual oscillator frequency */
#include <linux/build_bug.h>// helpers
#include <linux/io.h>       // for readl/writel
#include <linux/types.h>    // for u32, u8 (...) - kernel equivalent to user uin32_t ...

/* USER INCLUDE SECTION */
#include "utility.h"

/* HELPER MACROS */
#define CHECK_PIN(pin) (pin < 0 || pin > 57 ? -1 : 0)       

/* MACROS - configuration bits */
// PWM config
#define PWENx   1       // Channel x Enable: enabled (1)
#define MODEx   0       // Channel x Mode: PWM mode (0)

/* Channel 1 */
#define PWM_CTL_PWEN0   (1u << 0)   /* Channel 1 enable        */
#define PWM_CTL_MODE0   (1u << 1)   /* Channel 1 mode          */
#define PWM_CTL_RPTL0   (1u << 2)   /* Channel 1 repeat last   */
#define PWM_CTL_SBIT0   (1u << 3)   /* Channel 1 silence bit   */
#define PWM_CTL_POLA0   (1u << 4)   /* Channel 1 polarity      */
#define PWM_CTL_USEF0   (1u << 5)   /* Channel 1 use FIFO      */
#define PWM_CTL_CLRF0   (1u << 6)   /* Channel 1 clear FIFO    */
#define PWM_CTL_MSEN0   (1u << 7)   /* Channel 1 mark/space    */

/* Channel 2 */
#define PWM_CTL_PWEN1   (1u << 8)
#define PWM_CTL_MODE1   (1u << 9)
#define PWM_CTL_RPTL1   (1u << 10)
#define PWM_CTL_SBIT1   (1u << 11)
#define PWM_CTL_POLA1   (1u << 12)
#define PWM_CTL_USEF1   (1u << 13)
#define PWM_CTL_CLRF1   (1u << 14)
#define PWM_CTL_MSEN1   (1u << 15)

/* Convenience masks: all controllable bits of each channel */

#define PWM_CTL_CH0_MASK  (PWM_CTL_PWEN0 | PWM_CTL_MODE0 | PWM_CTL_RPTL0 | \
                           PWM_CTL_SBIT0 | PWM_CTL_POLA0 | PWM_CTL_USEF0 | \
                           PWM_CTL_CLRF0 | PWM_CTL_MSEN0)

#define PWM_CTL_CH1_MASK  (PWM_CTL_PWEN1 | PWM_CTL_MODE1 | PWM_CTL_RPTL1 | \
                           PWM_CTL_SBIT1 | PWM_CTL_POLA1 | PWM_CTL_USEF1 | \
                           PWM_CTL_CLRF1 | PWM_CTL_MSEN1)

// CLK config
#define ENAB    (1<<4)  // Enable the clock generator
#define SRC_OSC (1<<0)  // Clock source: GND default
#define BUSY    (1<<7)  // Busy bit: Clock generator is running (1)

/* TYPE DEFINITION */

typedef enum {
    CLK_SRC_GND0        = 0,   // GND
    CLK_SRC_OSCILLATOR  = 1,
    CLK_SRC_TESTDEBUG0  = 2,
    CLK_SRC_TESTDEBUG1  = 3,
    CLK_SRC_PLLA_PER    = 4,
    CLK_SRC_PLLC_PER    = 5,
    CLK_SRC_PLLD_PER    = 6,
    CLK_SRC_HDMI_AUX    = 7,

    // 8–15 map to GND too
    CLK_SRC_GND8        = 8,
    CLK_SRC_GND9        = 9,
    CLK_SRC_GND10       = 10,
    CLK_SRC_GND11       = 11,
    CLK_SRC_GND12       = 12,
    CLK_SRC_GND13       = 13,
    CLK_SRC_GND14       = 14,
    CLK_SRC_GND15       = 15,
} ClockSource;

/* VARIABLE INITIALIZATION */

// PWM pin configuration based on the datasheet
gpioPWM_config PWMconfig [NUM_PWM_PINS] = {
    {GPIO12, 12, ALT0, CHIP0, CH_0},
    {GPIO13, 13, ALT0, CHIP0, CH_1},
    {GPIO18, 18, ALT5, CHIP0, CH_0},
    {GPIO19, 19, ALT5, CHIP0, CH_1},
    {GPIO40, 40, ALT0, CHIP1, CH_0},
    {GPIO41, 41, ALT0, CHIP1, CH_1},
    {GPIO45, 45, ALT0, CHIP0, CH_1}
};

/* HELPER FUNCTIONS */
void init_clk(CLKRegister __iomem *clk)
{
    int timeout;
    u32 ctl, div;

    pr_alert("===================================\n");
    pr_alert("Initial CM_PWMCTL: 0x%08x\n", readl(&clk->CM_PWMCTL));
    pr_alert("Initial CM_PWMDIV: 0x%08x\n", readl(&clk->CM_PWMDIV));

    /* KILL the clock completely if on*/
    writel(CM_PASSWORD | 0x00, &clk->CM_PWMCTL);
    udelay(100);

    /* Wait for BUSY to clear */
    timeout = 10000;  /* 10ms timeout */
    while ((readl(&clk->CM_PWMCTL) & BUSY) && timeout-- > 0)
        udelay(1);
    
    if (timeout <= 0) {
        pr_err("ERROR: Clock won't stop! CM_PWMCTL=0x%08x\n", 
        readl(&clk->CM_PWMCTL));
    } else
        pr_alert("Clock stopped successfully\n");

    /* Try different clock sources - Source 6 is PLLD (500 MHz on RPi4) */
    pr_alert("Trying clock source 6 (PLLD)...\n");
    writel(CM_PASSWORD | CLK_SRC_PLLD_PER, &clk->CM_PWMCTL);
    udelay(100);

    /* Set divider for PLLD/26 = ~19.2 MHz (500/26 ≈ 19.2) */
    /*    Or use PLLD/4 = 125 MHz */
    /* Let's try divider = 4 for 125 MHz */
    writel(CM_PASSWORD | (4 << 12), &clk->CM_PWMDIV);
    udelay(100);

    div = readl(&clk->CM_PWMDIV);
    pr_alert("Set divider: 0x%08x (DIVI=%u)\n", div, (div >> 12) & 0xFFF);

    /* Enable clock with source 6 */
    writel(CM_PASSWORD | CLK_SRC_PLLD_PER | ENAB, &clk->CM_PWMCTL);
    udelay(100);

    /* Wait for BUSY (running) */
    timeout = 10000;
    while (!(readl(&clk->CM_PWMCTL) & BUSY) && timeout-- > 0)
        udelay(1);

    if (timeout <= 0)
        pr_err("ERROR: Clock won't start!\n");
    else
        pr_alert("Clock started successfully\n");

    /* Read back final state */
    ctl = readl(&clk->CM_PWMCTL);
    div = readl(&clk->CM_PWMDIV);

    pr_alert("Final CM_PWMCTL: 0x%08x\n", ctl);
    pr_alert("Final CM_PWMDIV: 0x%08x\n", div);
    pr_alert("  ENAB: %d\n", !!(ctl & ENAB));
    pr_alert("  BUSY: %d\n", !!(ctl & BUSY));
    pr_alert("  SRC: %u\n", ctl & 0xF);
    pr_alert("  DIVI: %u\n", (div >> 12) & 0xFFF);
    pr_alert("  Expected clock: PLLD/4 = 500MHz/4 = 125 MHz\n");
    pr_alert("===================================\n");

    /* If source 6 doesn't work, try source 1 (oscillator) */
    if (!(ctl & BUSY) || (ctl & 0xF) != CLK_SRC_PLLD_PER) {
        pr_alert("Source 6 failed, trying source 1 (oscillator) !!!\n");
        
        writel(CM_PASSWORD | 0x00, &clk->CM_PWMCTL);
        udelay(100);
        
        timeout = 10000;
        while ((readl(&clk->CM_PWMCTL) & BUSY) && timeout-- > 0)
            udelay(1);

        writel(CM_PASSWORD | CLK_SRC_OSCILLATOR, &clk->CM_PWMCTL);
        udelay(100);
        
        writel(CM_PASSWORD | (1 << 12), &clk->CM_PWMDIV);
        udelay(100);
        
        writel(CM_PASSWORD | CLK_SRC_OSCILLATOR | ENAB, &clk->CM_PWMCTL);
        udelay(100);

        ctl = readl(&clk->CM_PWMCTL);
        div = readl(&clk->CM_PWMDIV);
        
        pr_alert("Retry Final CM_PWMCTL: 0x%08x\n", ctl);
        pr_alert("Retry Final CM_PWMDIV: 0x%08x\n", div);
        pr_alert("  Expected clock: 19.2 MHz\n");
    }
}

/* This function uses the GPIORegister struct, since it configures the pin */
void SetGPIOFunction(GPIORegister* s_GPIORegister, PWM_GPIO GPIO_Pin, 
                     const FSELx functionCode, CLKRegister* s_CLK)
{
    int GPIO_PinNum = PWMconfig[GPIO_Pin].pin;
    if (!s_GPIORegister || CHECK_PIN(GPIO_PinNum) != 0)
        return;

    int GPFSELindex = (GPIO_PinNum/10);
    int startBit = (GPIO_PinNum%10)*3;
    int mask = 0b111 << startBit;
    u32 previousValue = s_GPIORegister->GPFSEL[GPFSELindex];

    s_GPIORegister->GPFSEL[GPFSELindex] = (previousValue & ~mask) | 
                                          (functionCode << startBit);
    
    pr_alert("GPIO%d set to function %d: GPFSEL%d = 0x%08x\n", 
             GPIO_PinNum, functionCode, GPFSELindex, 
             s_GPIORegister->GPFSEL[GPFSELindex]);

    // Check if mapping occured accordingly
    BUILD_BUG_ON(offsetof(PWMRegister, CTL)  != 0x00);
    BUILD_BUG_ON(offsetof(PWMRegister, STA)  != 0x04);
    BUILD_BUG_ON(offsetof(PWMRegister, DMAC) != 0x08);
    BUILD_BUG_ON(offsetof(PWMRegister, RNG1) != 0x10);
    BUILD_BUG_ON(offsetof(PWMRegister, DAT1) != 0x14);
    BUILD_BUG_ON(offsetof(PWMRegister, FIF1) != 0x18);
    BUILD_BUG_ON(offsetof(PWMRegister, RNG2) != 0x20);
    BUILD_BUG_ON(offsetof(PWMRegister, DAT2) != 0x24);
}

void SetPWM(PWMRegister __iomem *s_pwm,
            const PWM_GPIO gpio_id,
            int frequency,
            int duty_cycle,
            CLKRegister __iomem *clk_s_pwm)
{
    int gpio_pin;
    u8  channel;
    void __iomem *rng_reg;
    void __iomem *dat_reg;
    u32 ctl;
    u32 range;  // tick number
    u32 data;   // on-time tick number :  duty cycle

    gpio_pin = PWMconfig[gpio_id].pin;
    if (!s_pwm || CHECK_PIN(gpio_pin) != 0)
        return;

    channel = (u8)PWMconfig[gpio_id].channel;
    rng_reg = (channel == CH_0) ? &s_pwm->RNG1 : &s_pwm->RNG2;
    dat_reg = (channel == CH_0) ? &s_pwm->DAT1 : &s_pwm->DAT2;

    pr_alert("========================================\n");
    pr_alert("SetPWM: GPIO%d, Ch%d, freq=%d Hz, duty=%d%%\n",
             gpio_pin, channel, frequency, duty_cycle);

    ctl = readl(&s_pwm->CTL);
    pr_alert("Current CTL: 0x%08x\n", ctl);
    
    /* Disable channel - clear only PWENx bits (0 or 8)*/
    if (channel == CH_0)
    	 ctl &= ~PWM_CTL_PWEN0;
    else 
     	ctl &= ~PWM_CTL_PWEN1;  
   
    writel(ctl, &s_pwm->CTL);
    udelay(100);
    
    pr_alert("After disable: CTL = 0x%08x\n", readl(&s_pwm->CTL));

    /* Calculate values based on INPUT */
    if (frequency < 0) frequency = 1;
    if (duty_cycle < 0) duty_cycle = 0;
    else if (duty_cycle > 100) duty_cycle = 100;
    
    if (frequency == 0)
        return; // channel is already disabled
        
    range = (u32)(CLK_PWM / (u32)frequency);
        
    /* Check if range fits in 32-bit register f <= 0.04365 (~) = CLK_PWM/0xFFFFFFFF */
    if (range > 0xFFFFFFFF) {
        pr_warn("Frequency %d Hz too low. Range=%u exceeds 32-bit limit\n", 
                frequency, range);
        range = 0xFFFFFFFF;
    }
    
    /* 64-bit to avoid overflow for low frequencies on final result */
    data = ((u64)range * (u64)duty_cycle) / 100;
    
    pr_alert("CLK_PWM constant = %u Hz\n", CLK_PWM);
    pr_alert("Calculated: RNG=%u (0x%08x), DAT=%u (0x%08x)\n", 
             range, range, (u32)data, (u32)data);
    pr_alert("Expected freq = %u / %u = %u Hz\n", CLK_PWM, range, CLK_PWM / range);
    pr_alert("Expected duty = (%u * 100) / %u = %u%%\n",
             (u32)data, range, (u32)((data * 100) / range));

    /* Write values in memory */
    writel(range, rng_reg);
    writel(data, dat_reg);
    udelay(10);
    
    pr_alert("Readback: RNG%d=%u, DAT%d=%u\n",
             channel + 1, readl(rng_reg), channel + 1, readl(dat_reg));

    /* Configure CTL register properly for M/S (mark/space) mode */

    if (channel == CH_0) {
        ctl &= ~PWM_CTL_CH0_MASK;
        ctl |= PWM_CTL_PWEN0 | PWM_CTL_MSEN0;
    } else { /* CH_1 */
        ctl &= ~PWM_CTL_CH1_MASK;
        ctl |= PWM_CTL_PWEN1 | PWM_CTL_MSEN1;
    }


    pr_alert("Writing CTL: 0x%08x\n", ctl);
    writel(ctl, &s_pwm->CTL);
    udelay(10);

    /* Verify values */
    u32 final_ctl = readl(&s_pwm->CTL);
    u32 final_sta = readl(&s_pwm->STA);
    
    pr_alert("FINAL STATE:\n");
    pr_alert("  CTL = 0x%08x\n", final_ctl);
    pr_alert("  STA = 0x%08x\n", final_sta);
    pr_alert("  RNG%d = %u\n", channel + 1, readl(rng_reg));
    pr_alert("  DAT%d = %u\n", channel + 1, readl(dat_reg));
    pr_alert("========================================\n");
}
