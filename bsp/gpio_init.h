/*
 * bsp/gpio_init.h — Global GPIO configuration (BSP layer, driverlib wrapper).
 *
 * Phase 2 scope:
 *   - LEDs as outputs (off)
 *   - Buttons as pull-up inputs
 *
 * Other pins (motor, USS, RS485, ±15V, LT8490, HMI) are added to gpio_init()
 * in their own phases. Peripheral-function pins (UART, I2C, ADC, encoder
 * timer) are configured by their own bsp modules, not here.
 */

#ifndef BSP_GPIO_INIT_H_
#define BSP_GPIO_INIT_H_

/*
 * gpio_init() — set every managed pin's direction and safe initial level.
 * Call once at startup, after clock_init() (which already unlocked the I/O
 * via PMM_unlockLPM5).
 */
void gpio_init(void);

#endif /* BSP_GPIO_INIT_H_ */
