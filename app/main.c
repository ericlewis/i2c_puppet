#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>

#include <hardware/rtc.h>

#include "backlight.h"
#include "debug.h"
#include "gpioexp.h"
#include "interrupt.h"
#include "keyboard.h"
#include "puppet_i2c.h"
#include "reg.h"
#include "touchpad.h"
#include "usb.h"
#include "pi.h"

#if ENABLE_ESP32_SUPPORT
#include "esp32/esp32_comm.h"
#endif

// since the SDK doesn't support per-GPIO irq, we use this global irq and forward it
static void gpio_irq(uint gpio, uint32_t events)
{
//	printf("%s: gpio %d, events 0x%02X\r\n", __func__, gpio, events);
	touchpad_gpio_irq(gpio, events);
	gpioexp_gpio_irq(gpio, events);
}

// TODO: Microphone
int main(void)
{
	// The here order is important because it determines callback call order
	usb_init();

#ifndef NDEBUG
	debug_init();
#endif

	rtc_init();

	reg_init();

	backlight_init();

	gpioexp_init();

	keyboard_init();

	touchpad_init();

	interrupt_init();

	puppet_i2c_init();

	// For now, the `gpio` param is ignored and all enabled GPIOs generate the irq
	gpio_set_irq_enabled_with_callback(0xFF, 0, true, &gpio_irq);

	led_init();
	pi_power_init();

#if ENABLE_ESP32_SUPPORT
    esp32_auto_detect();
#endif

	pi_power_on(POWER_ON_FW_INIT);

#ifndef NDEBUG
	printf("Starting main loop\r\n");
#endif

	while (true) {
		__wfe();
	}

	return 0;
}
