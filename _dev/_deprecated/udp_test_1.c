// This implementation uses queues
// https://www.freertos.org/Embedded-RTOS-Queues.html
// queues support blocking and synchronization (since values are placed by value and not by reference)

// udp example: https://github.com/Ebiroll/qemu_esp32/blob/master/examples/19_udp/main/main.c

/*
    import dependencies and initialize utilities
*/
// standard c libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// O.S. libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// esp utils
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "apps/sntp/sntp.h"

#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

//esp logging
#include "esp_log.h"
static const char *TAG = "app";
static const char *TAG_RETREIVE = "retreive";
static const char *TAG_OUTPUT = "output";
int loop_count = 0;

// arduino
#include "Arduino.h"



/*
    inner-headers
*/
void setup();
void loop();
void app_main();
void milli_delay();

/*
    global constants
*/
#define TRIG_PIN (2)
#define ECHO_PIN (5)
#define DATA_STRING_SIZE ((5+3+8)* sizeof(char))// 5 for distance, 2 for delimterers, 8 for timestamp
#define PRODUCER_DELAY_MILLISECOND 500 // produce every X milliseconds
#define CONSUMER_DELAY_MILLISECOND 1000 // consume every X milliseconds
#define DATA_PER_OUTPUT 5 // k data per output

// for wifi
#define EXAMPLE_WIFI_SSID "Beach Bus" // iwgetid -r
#define EXAMPLE_WIFI_PASS "Waterfall"

// for communicating with UDP server
#define RECEIVER_PORT_NUM 3000
#define SENDER_PORT_NUM 3000
#define RECEIVER_IP_ADDR "192.168.1.105"
#define SENDER_IP_ADDR "127.0.0.1"

/*
    define global variables
*/
QueueHandle_t data_queue;

xTaskHandle time_updater_task_handle;
xTaskHandle producer_task_handle;
xTaskHandle consumer_task_handle;

time_t time_now;
struct tm time_now_timeinfo;
char strftime_buf[64];

// for wifi
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// for udp
int socket_fd;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    utilities
*/
void milli_delay(int milli){
    vTaskDelay(milli / portTICK_PERIOD_MS); // wait `milli` milliseconds
}
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
    time syncer
*/
static void request_time_update(void);
static void initialize_utilities(void);
static void initialize_non_volatile_storage(void);
static void update_time_with_sntp(void);
static void wait_until_time_updated(void);
static void start_wifi_connection(void);
static void stop_wifi_connection(void);
static void wait_until_wifi_connected(void);
static void initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);


static void task_update_internal_time_with_sntp( void *pvParameters ){
    const int sleep_sec = 60; // 10 seconds
    for( ;; )
    {
        ESP_LOGI(TAG_RETREIVE, "(!) Starting Update of Internal Wall-Clock Time Again...")

        request_time_update();
        //set_time_now_to_predefined();


        ESP_LOGI(TAG_RETREIVE, "update of wall clock time has completed. entering task wait for %d seconds", sleep_sec);
        vTaskDelay( sleep_sec * 1000 / portTICK_PERIOD_MS ); // wait / yield time to other tasks
    }
}
static void request_time_update(void){
    ESP_LOGI(TAG_RETREIVE, "retreiving time with SNTP protocol");

    start_wifi_connection();
    wait_until_wifi_connected();

    update_time_with_sntp();
    wait_until_time_updated();

    stop_wifi_connection();

}
static void initialize_utilities(void){
    ESP_LOGI(TAG_RETREIVE, "initializing utilities required for retreiving time with SNTP");
    initialize_non_volatile_storage();
    initialise_wifi();
}
static void initialize_non_volatile_storage(void){
    ESP_LOGI(TAG_RETREIVE, "initializing non-volatile storage")
    ESP_ERROR_CHECK( nvs_flash_init() ); // initialize non-volatile storage
}
static void update_time_with_sntp(void){
    ESP_LOGI(TAG_RETREIVE, "requesting update of time with SNTP");

    ESP_LOGI(TAG_RETREIVE, "starting SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // set mode to listen only, we will manually poll every hour in a seperate thread
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    ESP_LOGI(TAG_RETREIVE, "   SNTP polling opened. Waiting untill time updated...");
    wait_until_time_updated();
    ESP_LOGI(TAG_RETREIVE, "   time has been updated. closed SNTP polling.");
    sntp_stop();
    // sntp_stop

}
static void wait_until_time_updated(void){
    // init vars
    time_t now;
    struct tm timeinfo;

    // get the time
    time(&now);
    localtime_r(&now, &timeinfo);

    // test and block untill ready w/ max block of 10s
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (1990 - 1900) && ++retry < retry_count) { // consider all years above 1990 to be valid
        ESP_LOGI(TAG, "       Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}
static void start_wifi_connection(void){
    ESP_LOGI(TAG_RETREIVE, "starting WiFi radio and connection");
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}
static void stop_wifi_connection(void){
    ESP_LOGI(TAG_RETREIVE, "stopping WiFi radio and connection");
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_stop() );
}
static void wait_until_wifi_connected(void){
    ESP_LOGI(TAG_RETREIVE, "waiting untill wifi has connected");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY); // waits untill wifi is connected
    ESP_LOGI(TAG_RETREIVE, "wifi is now connected");
}
static void initialise_wifi(void){
    ESP_LOGI(TAG_RETREIVE, "initializing WIFI");
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG_RETREIVE, "   Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_LOGI(TAG_RETREIVE, "    WIFI initialized successfully");
}
static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}




/*
    producer
*/
int measure_distance(){
    // clear trigger
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    long duration = pulseIn(ECHO_PIN, HIGH, 1000000);

    // Calculating the distance
    int distance = duration*0.0343/2; // microns * meter / micron / 2; /2 because distance pulse travels is there AND back

    // print the distance
    //printf("%s: %d\n", "distance", distance);

    // return the distance
    return distance;
}
char* generate_data_string(){
    // get the distance
    int distance = measure_distance();

    // get the time
    time(&time_now);
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1); // set timezone to eastern
    tzset();
    localtime_r(&time_now, &time_now_timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%X", &time_now_timeinfo);

    // build the data string
    char *data_string = (char*)malloc(DATA_STRING_SIZE);
    sprintf(data_string, "%s;%d#", strftime_buf, distance); // add terminator to end

    // return the data
    return data_string;
}
void queue_data_string(){
    // retreive new datastring
    char *data_string = generate_data_string();
    //printf("%s\n", data_string);

    // add data to queue
    portBASE_TYPE queue_response = xQueueSendToBack(data_queue, data_string, 10); // allow up to 10 ticks of blocking
    if(queue_response != pdPASS){
        printf("%s\n", "(!) queue_send blocked task and waited for 10 ticks and was still not able to add to queue");
    }

    // free the data, since queue coppies the data and does not store referenc
    free(data_string);
}


/*
    consumer
*/
struct sockaddr_in sa,ra;
void initialize_socket(){
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if ( socket_fd < 0 )
    {

        printf("socket call failed");
        exit(0);

    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(SENDER_IP_ADDR);
    sa.sin_port = htons(SENDER_PORT_NUM);


    /* Bind the TCP socket to the port SENDER_PORT_NUM and to the current
    * machines IP address (Its defined by SENDER_IP_ADDR).
    * Once bind is successful for UDP sockets application can operate
    * on the socket descriptor for sending or receiving data.
    */
    if (bind(socket_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1)
    {
      printf("Bind to Port Number %d ,IP address %s failed\n",SENDER_PORT_NUM,SENDER_IP_ADDR /*SENDER_IP_ADDR*/);
      close(socket_fd);
      exit(1);
    }
    printf("Bind to Port Number %d ,IP address %s SUCCESS!!!\n",SENDER_PORT_NUM,SENDER_IP_ADDR);

    memset(&ra, 0, sizeof(struct sockaddr_in));
    ra.sin_family = AF_INET;
    ra.sin_addr.s_addr = inet_addr(RECEIVER_IP_ADDR);
    ra.sin_port = htons(RECEIVER_PORT_NUM);

}
void send_udp_packet(char* total_output_string){

    int sent_data; char data_buffer[80];

    strcpy(data_buffer,"Hello World");
    sent_data = lwip_sendto_r(socket_fd, data_buffer, sizeof("Hello World"),0,(struct sockaddr*)&ra,sizeof(ra));
    if(sent_data < 0)
    {
        printf("send failed\n");
        close(socket_fd);
        exit(2);
    }
    printf("data packet sent successfully!\n");
}
void output_data_from_queue(){
    // total data_string and queue_string
    char *total_output_string = "S#";
    char *queue_string = (char*)malloc(DATA_STRING_SIZE);

    // read data from queue
    //      read DATA_PER_OUTPUT times, untill queue is empty - then just finish
    int index;
    for(index=0; index < DATA_PER_OUTPUT; index++){
        portBASE_TYPE queue_data = xQueueReceive(data_queue, queue_string, 10); // wait up to 10 ticks of blocking
        if(queue_data){
            // data was found - append it to the total output string
            //printf("from queue: %s\n", queue_string);
            total_output_string = concat(total_output_string, queue_string);
            //printf("total output string now: %s\n", total_output_string);
        } else {
            // no data was found - break the loop by seting index = DATA_PER_OUTPUT
            //printf("%s\n", "(!) queue_receive was not able to receive data from queue even after 10 ticks of blocking");
            index=DATA_PER_OUTPUT;
        }
    }
    total_output_string = concat(total_output_string, "E");

    // output the total data from queue buffer
    printf("final combined output string: %s\n", total_output_string);
    printf("%s\n", "-------");

    // send the data with udp
    send_udp_packet(total_output_string);

    // free the data
    free(queue_string);
    free(total_output_string);
}



/*
    tasks
*/

static void task_producer( void *pvParameters )
{
    for( ;; )
    {
        ESP_LOGI(TAG_RETREIVE, "(!) measuring and recording distance...")
        queue_data_string();
        milli_delay(PRODUCER_DELAY_MILLISECOND);
    }
}
static void task_consumer( void *pvParameters )
{
    //const int sleep_count = 10;
    for( ;; )
    {
        output_data_from_queue();
        milli_delay(CONSUMER_DELAY_MILLISECOND);
    }
}

/*
    begin main methods
*/
void setup(){
    initArduino();

    // define pins
    pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
    pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input

    // init queue
    data_queue = xQueueCreate(10, DATA_STRING_SIZE ); // create queue capable of holding 10 data strings at a time
    if(data_queue != 0){
        printf("%s\n", "data_queue initialized successfully");
    }

    // start WiFi
    initialize_utilities();

    // start socket
    initialize_socket();

}

void app_main()
{
    setup();

    // start time syncer
    BaseType_t xReturned_updateTask;
    xReturned_updateTask = xTaskCreate(task_update_internal_time_with_sntp,  // pointer to function
                "time_update_task",        // Task name string for debug purposes
                8000,            // Stack depth as word
                NULL,           // function parameter (like a generic object)
                1,              // Task Priority (Greater value has higher priority)
                &time_updater_task_handle);  // Task handle
    if( xReturned_updateTask == pdPASS )
    {
        ESP_LOGI(TAG, "time updater task started successfully");
    }

    // wait untill the time is updated
    wait_until_time_updated();

    // start producer task
    BaseType_t xReturned_producer_task;
    xReturned_producer_task = xTaskCreate(task_producer,  // pointer to function
                "pulse_task",        // Task name string for debug purposes
                4000,            // Stack depth as word
                NULL,           // function parameter (like a generic object)
                2,              // Task Priority (Greater value has higher priority)
                &producer_task_handle);  // Task handle
    if( xReturned_producer_task == pdPASS )
    {
        ESP_LOGI(TAG, "producer task started successfully");
    }


    // start consumer task
    BaseType_t xReturned_consumer_task;
    xReturned_consumer_task = xTaskCreate(task_consumer,  // pointer to function
                "time_output_task",        // Task name string for debug purposes
                8000,            // Stack depth as word
                NULL,           // function parameter (like a generic object)
                2,              // Task Priority (Greater value has higher priority)
                &consumer_task_handle);  // Task handle
    if( xReturned_consumer_task == pdPASS )
    {
        ESP_LOGI(TAG, "consumer task started successfully");
    }


}
