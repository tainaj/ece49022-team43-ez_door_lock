/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "esp_netif.h"
#include "esp_netif_types.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#define PIN 2 //Pin will be replaced by switch GPIO PIN

// http header
const static char http_html_hdr[] =
		"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

// 404 header
const static char http_404_hdr[] =
        "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";

// http body html code
const static char http_index_hml[] =
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
		<title>EZ Door Lock</title><style>body{background-color: lightgray;font-size:24px;}</style></head>\
		<body><h1>EZ Door Lock Control Center</h1><a href=\"high\">OPEN</a><br><a href=\"low\">CLOSE</a></body></html>";

// WIFI ROUTER NAME
// WIFI PASSWORD
// CONNECTION TRIAL OF 5
// STATIC IP OF WEB SERVER
// SUBNET MASK ADDRESS
// GATE WAY ADDRESS
// DNS SERVER (GOOGLE)
#define EXAMPLE_ESP_WIFI_SSID      "TP-Link_B8F8"
#define EXAMPLE_ESP_WIFI_PASS      "09687493"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define STATIC_IP		"192.168.0.129"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.0.1"
#define DNS_SERVER		"8.8.8.8"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "web_server";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

static void http_server_netconn_serve(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);

		/* GET COMMAND */
		if (buflen >= 5 && strncmp("GET ",buf,4)==0) {

			//Parse URL
			char* path = NULL;
			char* line_end = strchr(buf, '\n');
			if( line_end != NULL )
			{
				//Extract the path from HTTP GET request
				path = (char*)malloc(sizeof(char)*(line_end-buf+1));
				int path_length = line_end - buf - strlen("GET ")-strlen("HTTP/1.1")-2;
				strncpy(path, &buf[4], path_length );
				path[path_length] = '\0';
				//Get remote IP address
				ip_addr_t remote_ip;
				u16_t remote_port;
				netconn_getaddr(conn, &remote_ip, &remote_port, 0);
				printf("[ "IPSTR" ] GET %s\n", IP2STR(&(remote_ip.u_addr.ip4)),path);
			}

			/* Send the HTML header
			 * subtract 1 from the size, since we dont send the \0 in the string
			 * NETCONN_NOCOPY: our data is const static, so no need to copy it
			 */
			bool bNotFound = false;
			if(path != NULL)
			{

				if (strcmp("/high",path)==0) {
					gpio_set_level(PIN,1);
				}
				else if (strcmp("/low",path)==0) {
					gpio_set_level(PIN,0);
				}
				else if (strcmp("/",path)==0)
				{
				
				}
				else
				{
					bNotFound = true;	// 404 Not found
				}
				
				free(path);
				path=NULL;
			}

			// Send HTTP response header
			if(bNotFound)
			{
				netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1,
					NETCONN_NOCOPY);
			}
			else
			{
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1,
					NETCONN_NOCOPY);
			}

			// Send HTML content
			netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1,
					NETCONN_NOCOPY);
		}

	}
	// Close the connection (server closes in HTTP)
	netconn_close(conn);

	// Delete the buffer (netconn_recv gives us ownership,
	// so we have to make sure to deallocate the buffer)
	netbuf_delete(inbuf);
}

static void http_server(void *pvParameters) {
	struct netconn *conn, *newconn;	//conn is listening thread, newconn is new thread for each client
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}

void app_main(void)
{
	//Wifi Station Code
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    //Wifi Station Code

    //GPIO Controller
    gpio_pad_select_gpio(PIN);
    gpio_set_direction(PIN, GPIO_MODE_OUTPUT);

    //server creation
    xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
}
