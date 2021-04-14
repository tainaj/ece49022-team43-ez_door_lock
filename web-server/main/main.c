#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_types.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#define MY_WIFI_SSID	"TYPE YOUR WIFI"
#define MY_WIFI_PASS	"TYPE YOUR PASSWORD"
#define STATIC_IP		"192.168.0.129"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.0.1"
#define DNS_SERVER		"8.8.8.8"

#define ESP_MAXIMUM_RETRY  5

#define ENABLE_STATIC_IP

static const char *TAG = "espressif";	//TAG for debug

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

int state = 0;
int OPEN_PIN = 21;

// http header
const static char http_html_hdr[] =
		"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

// 404 header
const static char http_404_hdr[] =
"HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";

// http body
const static char http_index_hml[] =
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
		<title>Control</title><style>body{background-color:lightblue;font-size:24px;}</style></head>\
		<body><h1>Control</h1><a href=\"high\">ON</a><br><a href=\"low\">OFF</a></body></html>";

// Has to implement this event_handler for WIFI station connection
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
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

    esp_netif_dhcpc_stop(0); // Don't run a DHCP client

	//Set static IP
	esp_netif_ip_info_t ipInfo;
	inet_pton(AF_INET, STATIC_IP, &ipInfo.ip);
	inet_pton(AF_INET, GATE_WAY, &ipInfo.gw);
	inet_pton(AF_INET, SUBNET_MASK, &ipInfo.netmask);
	esp_netif_set_ip_info(0, &ipInfo);

	//Set Main DNS server
	esp_netif_dns_info_t dnsInfo;
	inet_pton(AF_INET, DNS_SERVER, &dnsInfo.ip);
	esp_netif_set_dns_info(0, ESP_NETIF_DNS_MAIN,
			&dnsInfo);

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
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
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
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
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
                 MY_WIFI_SSID, MY_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 MY_WIFI_SSID, MY_WIFI_PASS);
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

		/* Is this an HTTP GET command? (only check the first 5 chars, since
		 there are other formats for GET, and we're keeping it very simple )*/
		if (buflen >= 5 && strncmp("GET ",buf,4)==0) {

			/*  sample:
			 * 	GET /l HTTP/1.1
				Accept: text/html, application/xhtml+xml, image/jxr,
				Referer: http://192.168.1.222/h
				Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3
				User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393
				Accept-Encoding: gzip, deflate
				Host: 192.168.1.222
				Connection: Keep-Alive
			 *
			 */
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
					gpio_set_level(OPEN_PIN,1);
				}
				else if (strcmp("/low",path)==0) {
					gpio_set_level(OPEN_PIN,0);
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

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	wifi_init_sta();

	//GPIO initialization
	gpio_pad_select_gpio(OPEN_PIN);
	gpio_set_direction(OPEN_PIN, GPIO_MODE_OUTPUT);

	ESP_LOGI(TAG, "Hello world! App is running ... ...\n");

	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
}
