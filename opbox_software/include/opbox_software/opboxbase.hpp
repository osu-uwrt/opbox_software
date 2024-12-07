#pragma once

/**
 * Base includes for opbox headers
 */

#include <iostream>


/**
 * Base settings for opbox
 */

#define OPBOX_IO_PRIMARY_BUZZER_FILE "/sys/class/leds/usr-buzzer/brightness"
#define OPBOX_IO_BACKUP_BUZZER_FILE "./test_files/test_buzzer"
#define OPBOX_IO_PRIMARY_LED_FILE "/sys/class/leds/usr-led/brightness"
#define OPBOX_IO_BACKUP_LED_FILE "./test_files/test_led"