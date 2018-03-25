
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define F_CPU (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000U)

#define LOW 0 // low boolean value
#define HIGH 1 // high boolean value
#define GPIO_TRIG 2 // hook "trigger" to pin 2
#define GPIO_ECHO 5 // hook "echo" to pin 5
#define GPIO_PIN_SEL ((1<<GPIO_ECHO)|(1<<GPIO_TRIG)) // bitmask of pins (e.g selection of pins)

#define MICRONS_PER_SECOND 1000000


/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
static const char *TAG = "app";
static const char *TAG_RETREIVE = "retreive";
static const char *TAG_OUTPUT = "output";


void display_pin_data(void);
void initialize_pins(void);
void milli_delay(int microns);
uint32_t gpio_read(int gpio);
void gpio_write(int gpio, int value);
void wait_for_pin_state(int gpio, int state, uint32_t start_cycle_count, uint32_t timeout_cycles);
uint32_t calculate_duration_of_echo_pulse();
void measure_distance();


/*
    variables globally defining task handlers
*/
xTaskHandle pulse_task_handle;
xTaskHandle display_task_handle;


/*
    clocks - microns conversion
*/
uint32_t clockCyclesPerMicrosecond(){
    return F_CPU / 1000000L;
}
uint32_t clockCyclesToMicroseconds(uint32_t a){
    return (a) / clockCyclesPerMicrosecond();
}
uint32_t microsecondsToClockCycles(uint32_t a){
    return (a) * clockCyclesPerMicrosecond();
}

/*
    data output methods
*/
void display_pin_data(){
    printf("TRIG, ECHO: %d,%d\n", gpio_get_level(GPIO_TRIG), gpio_get_level(GPIO_ECHO));
}
void display_pin_data_output(){
    printf("OUTPUT: TRIG, ECHO: %d,%d\n", gpio_get_level(GPIO_TRIG), gpio_get_level(GPIO_ECHO));
}

/*
    initialization methods
*/
void initialize_pins(){
    /*
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; // disable interupts
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT; // mode both input and output
    io_conf.pin_bit_mask = GPIO_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    */

  pinMode(GPIO_ECHO, INPUT);
}

/*
    helper methods
*/
void milli_delay(int milli){
    vTaskDelay(milli / portTICK_PERIOD_MS); // wait `milli` milliseconds
}
void micron_delay(int micron){
    const uint32_t start_cycle_count = xthal_get_ccount();
    const uint32_t cycles = microsecondsToClockCycles(micron); // convert microns to cycles
    uint32_t time_taken = 0;
    //printf("wait for %d microns\n", micron);
    while (time_taken < cycles) {
        time_taken = xthal_get_ccount() - start_cycle_count; // update time taken
    }
}

/*
    read / write simplifiers
*/
uint32_t gpio_read(int gpio){
    return gpio_get_level(gpio);
}
void gpio_write(int gpio, int value){
    ESP_ERROR_CHECK(gpio_set_level(gpio, value));
}

/*
    calculate distance, implement pulseIn like https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/wiring_pulse.c
*/
/*
void wait_for_pin_state(int gpio, int state, uint32_t start_cycle_count, uint32_t timeout_cycles){
    while (gpio_read(gpio) != (state)) {
        const uint32_t time_taken = xthal_get_ccount() - start_cycle_count;
        if (time_taken > timeout_cycles) {
            printf("%s: %d %d\n", "wait_for_pin_state timeout exceeded", timeout_cycles, time_taken);
            return;
        }
    }
}
uint32_t calculate_duration_of_echo_pulse(){

    //    1. find begining of pulse (wait_for_low(), wait_for_high())
    //    2. determine start time after finding beginning
    //    3. find end of pulse (wait_for_low())

    printf("%s\n", "start calculation of echo_pulse duration");
    const uint32_t timeout_cycles = microsecondsToClockCycles(10*MICRONS_PER_SECOND); // max timeout to 10 seconds
    const uint32_t start_cycle_count = xthal_get_ccount();
    wait_for_pin_state(GPIO_ECHO, LOW, start_cycle_count, timeout_cycles); // WAIT FOR LOW STATE
    wait_for_pin_state(GPIO_ECHO, HIGH, start_cycle_count, timeout_cycles); // WAIT FOR HIGH STATE
    const uint32_t pulse_start_cycle_count = xthal_get_ccount();  //
    wait_for_pin_state(GPIO_ECHO, LOW, start_cycle_count, timeout_cycles); // WAIT FOR LOW STATE
    const uint32_t end_cycle_count = xthal_get_ccount();
    const uint32_t clockcycle_duration = end_cycle_count - pulse_start_cycle_count;
    const uint32_t micron_duration = clockCyclesToMicroseconds(clockcycle_duration);

    printf("micron duration: %d\n", micron_duration);
    return micron_duration;
}
void measure_distance(){
    // calculate how long it takes pulse to return; reads the echoPin, returns the sound wave travel time in microseconds
    uint32_t duration = calculate_duration_of_echo_pulse();
}
*/

/*
    pulse sending
*/
void send_pulse(){
    ESP_LOGI(TAG_OUTPUT, "pulse task started");

    // ensure that trigger is low
    gpio_write(GPIO_TRIG, LOW);
    micron_delay(2);

    // seld pulse; set trigger to HIGH for 10micron
    printf("%s\n", "triggering a micron pulse");
    gpio_write(GPIO_TRIG, HIGH);
    milli_delay(10);
    printf("%s\n", "done");
    gpio_write(GPIO_TRIG, LOW);
    milli_delay(10);
    printf("%s\n", "again try");
    gpio_write(GPIO_TRIG, HIGH);
    milli_delay(10);
    printf("%s\n", "again done");
    gpio_write(GPIO_TRIG, LOW);
    //display_pin_data();
}


/*
    tasks
*/
static void task_periodically_pulse( void *pvParameters )
{
    //const int sleep_sec = 60; // 10 seconds
    for( ;; )
    {
        ESP_LOGI(TAG_RETREIVE, "(!) sending pulse...")
        send_pulse();
        milli_delay(1000); // 1 second delay
    }
}
static void display_data( void *pvParameters)
{
    //ESP_LOGI(TAG_OUTPUT, "(!) Displaying Internal Time...")
    display_pin_data_output();
    vTaskDelete( NULL ); // exit self cleanly - https://www.freertos.org/implementing-a-FreeRTOS-task.html
}
static void display_data_loop( void *pvParameters )
{
    //const int sleep_count = 10;
    for( ;; )
    {
        //ESP_LOGI(TAG_OUTPUT, "starting display time task asyncronously");
        xTaskCreate(display_data,  // pointer to function
            "data_display_task",        // Task name string for debug purposes
            4000,            // Stack depth as word
            NULL,           // function parameter (like a generic object)
            1,              // Task Priority (Greater value has higher priority)
            NULL);          // Task handle can be ignored, task kills self
        milli_delay(1);
        //micron_delay(sleep_count); // wait / yield time to other tasks
    }
}



void app_main() {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    initialize_pins();

    /*
        create updater task - updates time w/ SNTP every hour
    */
    BaseType_t xReturned_pulseTask;
    xReturned_pulseTask = xTaskCreate(task_periodically_pulse,  // pointer to function
                "pulse_task",        // Task name string for debug purposes
                4000,            // Stack depth as word
                NULL,           // function parameter (like a generic object)
                1,              // Task Priority (Greater value has higher priority)
                &pulse_task_handle);  // Task handle
    if( xReturned_pulseTask == pdPASS )
    {
        ESP_LOGI(TAG, "pulse task started successfully");
    }


    /*
        create display task - displays time every 10 ms
    */
    BaseType_t xReturned_outputTask;
    xReturned_outputTask = xTaskCreate(display_data_loop,  // pointer to function
                "time_output_task",        // Task name string for debug purposes
                8000,            // Stack depth as word
                NULL,           // function parameter (like a generic object)
                1,              // Task Priority (Greater value has higher priority)
                &display_task_handle);  // Task handle
    if( xReturned_outputTask == pdPASS )
    {
        ESP_LOGI(TAG, "time output task started successfully");
    }


}
