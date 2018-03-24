
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


#define GPIO_OUTPUT_IO_0 12
#define GPIO_OUTPUT_IO_1 27
#define GPIO_OUTPUT_PIN_SEL ((1<<GPIO_OUTPUT_IO_0) | (1<<GPIO_OUTPUT_IO_1))

/*
    data output methods
*/
void display_input_pin_data(){
    printf("GPIO_OUTPUT_IO_0: %d\n", gpio_get_level(GPIO_OUTPUT_IO_0)) ; // 0 NOT expected
    printf("GPIO_OUTPUT_IO_1: %d\n", gpio_get_level(GPIO_OUTPUT_IO_1)) ; // 0 NOT expected
}

/*
    initialization methods
*/
void initialize_pins(){
    // initialize output pins
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1 ;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // initialize input pins
    // TODO
}

void app_main() {
    initialize_pins();

    // SET GPIO to 0
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_0, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_1, 0));
    display_input_pin_data();

    printf("----------------\n") ;

    // SET GPIO to 1
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_0, 1));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_1, 1));
    display_input_pin_data();

    // WAIT
    while(1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        display_input_pin_data();
    }
}
