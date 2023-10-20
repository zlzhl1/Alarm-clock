/* TTGO Demo example for 159236

*/
#include <driver/gpio.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client.h"

#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <nvs_flash.h>

#include "networking.h"
#include "fonts.h"
#include "graphics.h"
#include "image_wave.h"
#include "demos.h"
#include "graphics3d.h"
#include "input_output.h"
#include "time.h"

#define PAD_START 3
#define PAD_END 5

#define SHOW_PADS

void wifi_settings_menu()
{
    int sel = 0;
    while (1)
    {
        char *entries[] = {"Choose AP", "SSID", "Username",
                           "Password", "Back"};
        sel = demo_menu("Wifi Menu", sizeof(entries) / sizeof(char *), entries, sel);
        switch (sel)
        {
        case 0:
            wifi_scan(1);
            break;
        case 1:
            edit_stored_string("ssid", "SSID");
            break;
        case 2:
            edit_stored_string("username", "Username");
            break;
        case 3:
            edit_stored_string("password", "Password");
            break;
        case 4:
            return;
        }
    }
}

void led_menu()
{
    int sel = 0;
    while (1)
    {
        char *entries[] = {"MQTT", "Circles", "Numbers", "Cube", "Back"};
        sel = demo_menu("Leds Menu", sizeof(entries) / sizeof(char *), entries, sel);
        switch (sel)
        {
        case 0:
            mqtt_leds();
            break;
        case 1:
            led_circles();
            break;
        case 2:
            led_numbers();
            break;
        case 3:
            led_cube();
        case 4:
            return;
        }
    }
}

void wifi_menu()
{
    int sel = 0;
    while (1)
    {
        int connected = wifi_connected();
        char *entries[] = {"Scan", connected ? "Disconnect" : "Connect", "Access Point",
                           "Settings", "Back"};
        sel = demo_menu("Wifi Menu", sizeof(entries) / sizeof(char *), entries, sel);
        switch (sel)
        {
        case 0:
            wifi_scan(0);
            break;
        case 1:
            if (connected)
                wifi_disconnect();
            else
                wifi_connect(0);
            break;
        case 2:
            wifi_ap();
            break;
        case 3:
            wifi_settings_menu();
            break;
        case 4:
            return;
        }
    }
}
void graphics_menu()
{
    int sel = 0;
    while (1)
    {
        char *entries[] = {"Boids", "Life", "Image Wave", "Spaceship", get_orientation() ? "Landscape" : "Portrait", "Back"};
        sel = demo_menu("Graphics Menu", sizeof(entries) / sizeof(char *), entries, sel);
        switch (sel)
        {
        case 0:
            boids_demo();
            break;
        case 1:
            life_demo();
            break;
        case 2:
            image_wave_demo();
            break;
        case 3:
            spaceship_demo();
            break;
        case 4:
            set_orientation(1 - get_orientation());
            break;
        case 5:
            return;
        }
    }
}

void network_menu()
{
    int sel = 0;
    while (1)
    {
        char *entries[] = {"Wifi", "MQTT", "Time", "Web Server", "Web Client", "Back"};
        sel = demo_menu("Network Menu", sizeof(entries) / sizeof(char *), entries, sel);
        switch (sel)
        {
        case 0:
            wifi_menu();
            break;
        case 1:
            mqtt();
            break;
        case 2:
            time_demo();
            break;
        case 3:
            webserver();
            break;
        case 4:
            web_client();
            break;
        case 5:
            return;
        }
    }
}

int year, month, day, hour, minute;
static void my_mqtt_callback(int event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    if (event_id == MQTT_EVENT_CONNECTED)
    {
        esp_mqtt_client_handle_t client = event->client;
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(network_interface, &ip_info);
        // Publish a message to the MQTT topic
        char buf[64];
        snprintf(buf, sizeof(buf), "Connected:" IPSTR, IP2STR(&ip_info.ip));
        esp_mqtt_client_publish(client, "/topic/a159236", buf, 0, 1, 0);
        // Subscribe to the MQTT topic
        esp_mqtt_client_subscribe(client, "/topic/a159236", 0);
    }
    else if (event_id == MQTT_EVENT_DATA)
    {
        event->data[event->data_len] = 0;
        snprintf(network_event, sizeof(network_event), "MQTT_DATA\n%s\n", event->data);
        sscanf(event->data, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
    }
}
void app_main()
{
    // initialise button handling
    input_output_init();
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // ===== Set time zone to NZ time using custom daylight savings rule======
    // if you are anywhere else in the world, this will need to be changed
    setenv("TZ", "NZST-12:00:00NZDT-13:00:00,M9.5.0,M4.1.0", 0);
    tzset();
    // initialise graphics and lcd display
    graphics_init();
    cls(0);
    wifi_connect(0);
    mqtt_connect(my_mqtt_callback);
    if (xEventGroupGetBits(network_event_group) & CONNECTED_BIT)
    {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(network_interface, &ip_info);
    }

    char time_str[256];
    int sntp_status = 0;
    time_t time_now;
    struct tm *tm_info;

    while (1)
    {
        cls(0);
        setFont(FONT_DEJAVU18);
        setFontColour(0, 0, 0);

        // Draw a colored rectangle at the top of the screen
        draw_rectangle(3, 0, display_width, 18, rgbToColour(220, 220, 0));
        print_xy("Time\n", 5, 3);
        setFont(FONT_UBUNTU16);
        setFontColour(255, 255, 255);
        gprintf(network_event);
        if (xEventGroupGetBits(network_event_group) & CONNECTED_BIT)
        {
            if (sntp_status == 0)
            {
                esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                esp_sntp_setservername(0, "pool.ntp.org");
                esp_sntp_init();
                sntp_status = 1;
            }
            time(&time_now);
            tm_info = localtime(&time_now);
            setFont(FONT_DEJAVU24);
            // Change font color based on the year
            if (tm_info->tm_year < (2016 - 1900))
                setFontColour(255, 0, 0);
            else
                setFontColour(0, 255, 0);
            bool flag = true;
            int temp = 0;
            if (flag == true && year == (tm_info->tm_year) + 1900 && month == (tm_info->tm_mon) + 1 && day == tm_info->tm_mday && hour == tm_info->tm_hour && minute == tm_info->tm_min)
            {
                temp = tm_info->tm_min;
                // Initialize GPIO for specific actions
                input_output_init();
                gpio_set_direction(1, GPIO_MODE_OUTPUT);
                gpio_set_level(GPIO_NUM_10, 1);
                // Perform actions while waiting for user input
                while (get_input() != LEFT_DOWN && get_input() != RIGHT_DOWN)
                {
                    cls(rgbToColour(255, 0, 0));
                    flip_frame();
                    uint64_t time = esp_timer_get_time() + 300000;
                    while (esp_timer_get_time() < time)
                        ;
                    cls(rgbToColour(0, 255, 0));
                    flip_frame();
                    time = esp_timer_get_time() + 300000;
                    while (esp_timer_get_time() < time)
                        ;
                }
                flag = false;

                gpio_set_level(GPIO_NUM_21, 0);
            }
            // Format and display time
            if (temp + 1 == tm_info->tm_min)
            {
                flag = true;
            }
            if (tm_info->tm_hour == 12)
            {
                snprintf(time_str, 64, "\n%2d:%02d:%02d PM", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
            else if (tm_info->tm_hour > 12)
            {
                snprintf(time_str, 64, "\n%2d:%02d:%02d PM", tm_info->tm_hour - 12, tm_info->tm_min, tm_info->tm_sec);
            }
            else
            {
                snprintf(time_str, 64, "\n%2d:%02d:%02d AM", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
            print_xy(time_str, 45, 35);
        }
        flip_frame();
    }
    esp_sntp_stop();
}
