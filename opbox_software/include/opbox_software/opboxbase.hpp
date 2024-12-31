#pragma once

/**
 * Base includes for opbox headers
 */

#include <iostream>

#ifndef INSTALL_LOCATION
#define INSTALL_LOCATION "/usr/local"
#endif


/**
 * Base settings for opbox
 */

#define OPBOX_IO_PRIMARY_BUZZER_FILE "/sys/class/leds/usr-buzzer/brightness"
#define OPBOX_IO_BACKUP_BUZZER_FILE "share://test_files/test_buzzer"
#define OPBOX_IO_PRIMARY_LED_FILE "/sys/class/leds/usr-led/brightness"
#define OPBOX_IO_BACKUP_LED_FILE "share://test_files/test_led"
#define OPBOX_IO_DEFAULT_FAKE_GPIO_FILE "share://test_gpio"
