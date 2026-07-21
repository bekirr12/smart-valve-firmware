/*
 * bsp/gpio_init.c — Global GPIO configuration implementation.
 *
 * Phase 2 scope: configure the LEDs (outputs, off) and the buttons
 * (pull-up inputs). Other pins (motor, USS, RS485, ±15V, LT8490, HMI)
 * are added to this function in their own phases.
 *
 * The LCD_C module is disabled by default after a BOR reset, so the
 * LCD-shared pins already act as GPIO once LOCKLPM5 is cleared (done in
 * clock_init).
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/gpio_init.h"

void gpio_init(void)
{
    /* --- LEDs: outputs, start OFF (active-high) --------------------- */
    GPIO_setAsOutputPin(LED1_PORT, LED1_PIN);
    GPIO_setAsOutputPin(LED2_PORT, LED2_PIN);
    GPIO_setAsOutputPin(LED3_PORT, LED3_PIN);
    GPIO_setOutputLowOnPin(LED1_PORT, LED1_PIN);
    GPIO_setOutputLowOnPin(LED2_PORT, LED2_PIN);
    GPIO_setOutputLowOnPin(LED3_PORT, LED3_PIN);

    /* --- Buttons: inputs with pull-up (active-low) ------------------
     * Pull-up means the pin idles HIGH; pressing the button pulls it to
     * GND (reads LOW). This avoids a floating input when not pressed.
     */
    GPIO_setAsInputPinWithPullUpResistor(BTN_DOWN_PORT,   BTN_DOWN_PIN);
    GPIO_setAsInputPinWithPullUpResistor(BTN_SELECT_PORT, BTN_SELECT_PIN);
    GPIO_setAsInputPinWithPullUpResistor(BTN_LEFT_PORT,   BTN_LEFT_PIN);
    GPIO_setAsInputPinWithPullUpResistor(BTN_UP_PORT,     BTN_UP_PIN);
    GPIO_setAsInputPinWithPullUpResistor(BTN_RIGHT_PORT,  BTN_RIGHT_PIN);

    /* --- HMI screen control lines (Phase 12) ------------------------
     * AUDIO-PA-EN is active-low for audio, so drive it HIGH to keep the
     * screen's audio amplifier OFF (saves a little current and avoids
     * spurious noise). The others start low.
     * (P7.0/P7.1 are the HMI UART and are configured in uart_hmi_init.)
     */
    GPIO_setAsOutputPin(HMI_AUDIO_EN_PORT, HMI_AUDIO_EN_PIN);
    GPIO_setOutputHighOnPin(HMI_AUDIO_EN_PORT, HMI_AUDIO_EN_PIN);  /* audio off */

    GPIO_setAsOutputPin(HMI_SPK_PORT, HMI_SPK_PIN);
    GPIO_setOutputLowOnPin(HMI_SPK_PORT, HMI_SPK_PIN);

    GPIO_setAsOutputPin(HMI_PE9_PORT, HMI_PE9_PIN);
    GPIO_setOutputLowOnPin(HMI_PE9_PORT, HMI_PE9_PIN);

    GPIO_setAsOutputPin(HMI_PE8_PORT, HMI_PE8_PIN);
    GPIO_setOutputLowOnPin(HMI_PE8_PORT, HMI_PE8_PIN);
}
