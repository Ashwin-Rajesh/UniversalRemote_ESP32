/* MDNS-SD Query and advertise Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "protocol_examples_common.h"
#include "mdns.h"
#include "driver/gpio.h"
#include "netdb.h"


#define EXAMPLE_MDNS_INSTANCE CONFIG_MDNS_INSTANCE
#define EXAMPLE_BUTTON_GPIO     0

#define WIFI_SSID "OPTERNA-7D26"
#define WIFI_PASSWORD "12345678"

#define MDNS_SERVICE_NAME "UniversalRemoteTest"

static const char *TAG = "mdns-test";
static char* generate_hostname(void);

#if CONFIG_MDNS_RESOLVE_TEST_SERVICES == 1
static void  query_mdns_host_with_gethostbyname(char * host);
static void  query_mdns_host_with_getaddrinfo(char * host);
#endif

esp_netif_t *netif_obj;

static void wifi_event_handler(void* event_handler, esp_event_base_t event, int32_t event_id, void* event_data)
{
	wifi_ap_record_t ap_record;

	switch(event_id)
	{
	case WIFI_EVENT_STA_CONNECTED:

		esp_wifi_sta_get_ap_info(&ap_record);

		ESP_LOGI(TAG, " Station connected to SSID : %s", ap_record.ssid);

		break;

	case WIFI_EVENT_STA_WPS_ER_FAILED:

		ESP_LOGI(TAG, " WPS authentication failed");

		break;

	case WIFI_EVENT_STA_WPS_ER_TIMEOUT:

		ESP_LOGI(TAG, " WPS authentication timed out");

		break;

	case WIFI_EVENT_STA_WPS_ER_SUCCESS:

		esp_wifi_sta_get_ap_info(&ap_record);

		ESP_LOGI(TAG, " Station connected to SSID : %s", ap_record.ssid);

		break;

	default:

		ESP_LOGI(TAG, "Some other %d event was detected", event_id);
	}
}

static void got_ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(netif_obj, &ip_info);

    ESP_LOGI(TAG, "IP : " IPSTR " Gateway : " IPSTR " Netmask : " IPSTR, IP2STR(&ip_info.ip), IP2STR(&ip_info.gw), IP2STR(&ip_info.netmask));
}

void setup_wifi(void)
{
	esp_event_loop_create_default();

	esp_netif_init();

	netif_obj = esp_netif_create_default_wifi_sta();

	wifi_init_config_t wifi_init_struct = WIFI_INIT_CONFIG_DEFAULT();

	esp_wifi_init(&wifi_init_struct);

	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_wifi_start();
}

void wifi_connect()
{
    wifi_config_t wifi_struct = {
    		.sta = {
    				.ssid = WIFI_SSID,
		            .password = WIFI_PASSWORD,
		            .bssid_set = false
    		}
    };


    ESP_LOGI(TAG, " Trying to connect. SSID : %s, Password : %s", wifi_struct.sta.ssid, wifi_struct.sta.password);

	esp_wifi_set_config(WIFI_IF_STA, &wifi_struct);

	ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));
}

static void initialise_mdns(void)
{
    char* hostname = generate_hostname();
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(EXAMPLE_MDNS_INSTANCE) );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"u","user"},
        {"p","password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add(MDNS_SERVICE_NAME, "_http", "_tcp", 80, serviceTxtData, 3) );
    //add another TXT item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "u", "admin") );
    free(hostname);
}

/* these strings match tcpip_adapter_if_t enumeration */
static const char * if_str[] = {"STA", "AP", "ETH", "MAX"};

/* these strings match mdns_ip_protocol_t enumeration */
static const char * ip_protocol_str[] = {"V4", "V6", "MAX"};

static void mdns_print_results(mdns_result_t * results){
    mdns_result_t * r = results;
    mdns_ip_addr_t * a = NULL;
    int i = 1, t;
    while(r){
        printf("%d: Interface: %s, Type: %s\n", i++, if_str[r->tcpip_if], ip_protocol_str[r->ip_protocol]);
        if(r->instance_name){
            printf("  PTR : %s\n", r->instance_name);
        }
        if(r->hostname){
            printf("  SRV : %s.local:%u\n", r->hostname, r->port);
        }
        if(r->txt_count){
            printf("  TXT : [%u] ", r->txt_count);
            for(t=0; t<r->txt_count; t++){
                printf("%s=%s; ", r->txt[t].key, r->txt[t].value?r->txt[t].value:"NULL");
            }
            printf("\n");
        }
        a = r->addr;
        while(a){
            if(a->addr.type == ESP_IPADDR_TYPE_V6){
                printf("  AAAA: " IPV6STR "\n", IPV62STR(a->addr.u_addr.ip6));
            } else {
                printf("  A   : " IPSTR "\n", IP2STR(&(a->addr.u_addr.ip4)));
            }
            a = a->next;
        }
        r = r->next;
    }

}

static void query_mdns_service(const char * service_name, const char * proto)
{
    ESP_LOGI(TAG, "Query PTR: %s.%s.local", service_name, proto);

    mdns_result_t * results = NULL;

    esp_err_t err = mdns_query_ptr(service_name, proto, 3000, 20,  &results);
    if(err){
        ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
        return;
    }
    if(!results){
        ESP_LOGW(TAG, "No results found!");
        return;
    }

    mdns_print_results(results);
    mdns_query_results_free(results);
}

static void query_mdns_host(const char * host_name)
{
    ESP_LOGI(TAG, "Query A: %s.local", host_name);

    struct esp_ip4_addr addr;
    addr.addr = 0;

    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGW(TAG, "%s: Host was not found!", esp_err_to_name(err));
            return;
        }
        ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
}

static void initialise_button(void)
{
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BIT64(EXAMPLE_BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
}

static void check_button(void)
{
    static bool old_level = true;
    bool new_level = gpio_get_level(EXAMPLE_BUTTON_GPIO);
    if (!new_level && old_level) {
        query_mdns_host("esp32");
        query_mdns_service("_arduino", "_tcp");
        query_mdns_service("_http", "_tcp");
        query_mdns_service("_printer", "_tcp");
        query_mdns_service("_ipp", "_tcp");
        query_mdns_service("_afpovertcp", "_tcp");
        query_mdns_service("_smb", "_tcp");
        query_mdns_service("_ftp", "_tcp");
        query_mdns_service("_nfs", "_tcp");
    }
    old_level = new_level;
}

static void mdns_example_task(void *pvParameters)
{
#if CONFIG_MDNS_RESOLVE_TEST_SERVICES == 1
    /* Send initial queries that are started by CI tester */
    query_mdns_host("tinytester");
    query_mdns_host_with_gethostbyname("tinytester-lwip.local");
    query_mdns_host_with_getaddrinfo("tinytester-lwip.local");
#endif

    while(1) {
        check_button();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setup_wifi();

    wifi_connect();

    initialise_mdns();

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    initialise_button();
    xTaskCreate(&mdns_example_task, "mdns_example_task", 2048, NULL, 5, NULL);
}

/** Generate host name based on sdkconfig, optionally adding a portion of MAC address to it.
 *  @return host name string allocated from the heap
 */
static char* generate_hostname(void)
{
#ifndef CONFIG_MDNS_ADD_MAC_TO_HOSTNAME
    return strdup(CONFIG_MDNS_HOSTNAME);
#else
    uint8_t mac[6];
    char   *hostname;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", CONFIG_MDNS_HOSTNAME, mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
#endif
}

#if CONFIG_MDNS_RESOLVE_TEST_SERVICES == 1
/**
 *  @brief Executes gethostbyname and displays list of resolved addresses.
 *  Note: This function is used only to test advertised mdns hostnames resolution
 */
static void  query_mdns_host_with_gethostbyname(char * host)
{
    struct hostent *res = gethostbyname(host);
    if (res) {
        unsigned int i = 0;
        while (res->h_addr_list[i] != NULL) {
            ESP_LOGI(TAG, "gethostbyname: %s resolved to: %s", host, inet_ntoa(*(struct in_addr *) (res->h_addr_list[i])));
            i++;
        }
    }
}

/**
 *  @brief Executes getaddrinfo and displays list of resolved addresses.
 *  Note: This function is used only to test advertised mdns hostnames resolution
 */
static void  query_mdns_host_with_getaddrinfo(char * host)
{
    struct addrinfo hints;
    struct addrinfo * res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (!getaddrinfo(host, NULL, &hints, &res)) {
        while (res) {
            ESP_LOGI(TAG, "getaddrinfo: %s resolved to: %s", host,
                     res->ai_family == AF_INET?
                     inet_ntoa(((struct sockaddr_in *) res->ai_addr)->sin_addr):
                     inet_ntoa(((struct sockaddr_in6 *) res->ai_addr)->sin6_addr));
            res = res->ai_next;
        }
    }
}
#endif
