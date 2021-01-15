#include "Arduino.h"
#include "WiFi.h"
#include "ESPmDNS.h"

#include "NetworkHandler.h"

#include "nvs_flash.h"

// Handler function for http get requests for raw messages
esp_err_t WiFiHandler::http_get_handler(httpd_req_t* req)
{
    WiFiled->blink_once();

    String resp;

	ESP_LOGI(TAG, " Got a get request.");

    IRled->start_blinking();
    
    esp_err_t ret = receiver->get_raw(resp);

    IRled->stop_blinking();

    if(ret == ESP_FAIL)
        resp = "-1";

    ESP_LOGI(TAG, "Sending response :%s", resp.c_str());

	httpd_resp_send(req, resp.c_str(), resp.length());

	return ESP_OK;
}

// Handler function for http post messages for raw messages
esp_err_t WiFiHandler::http_post_handler(httpd_req_t *req)
{
    WiFiled->blink_once();
    
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[MAX_STR_LEN];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size); 

    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to / :%s", content);

    IRled->blink_once();
    
    esp_err_t str_ret = sender->send_raw(content);
    
    char resp[100];
    
    // Send response to client
    if(str_ret == ESP_FAIL)
        strcpy(resp, "Invalid format");
    else
        strcpy(resp, "Success");

    ESP_LOGI(TAG, "Sending response :%s", resp);
    
    httpd_resp_send(req, resp, strlen(resp));

    return ESP_OK;
}

// Handler function for http post message type, for AC messages
esp_err_t WiFiHandler::http_ac_post_handler(httpd_req_t *req)
{
    WiFiled->blink_once();
    
    char content[MAX_STR_LEN];

    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    esp_err_t ret = httpd_req_recv(req, content, recv_size);

    if (ret <= 0) 
    {
        // Send a request timed out error code (408) if connection timedout
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            httpd_resp_send_408(req);
        // To close the socket, return ESP_FAIL
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to /ac :%s", content);

    IRled->blink_once();

    esp_err_t str_ret = sender->send_ac(content);
    
    char resp[100];
    
    // Send response to client
    if(str_ret == ESP_FAIL)
        strcpy(resp, "Invalid format");
    else
        strcpy(resp, "Success");

    ESP_LOGI(TAG, "Sending response :%s", resp);
    
    httpd_resp_send(req, resp, strlen(resp));

    return ESP_OK;
}

// Scans for available wifi networks and 
esp_err_t WiFiHandler::http_scan_handler(httpd_req_t *req)
{
    WiFiled->start_blinking();

    int n = WiFi.scanNetworks();
        if (n == 0) {
        ESP_LOGI(TAG, "no networks found");
    } else {
        ESP_LOGI(TAG, "%d", n);
        ESP_LOGI(TAG, " networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            ESP_LOGI(TAG, "%d : %s", i, WiFi.SSID(i));
            delay(10);
        }
    }

    WiFiled->stop_blinking();
    
    String response;
    response.reserve(MAX_STR_LEN);

    for(int i = 0; i < n; ++i) {
        response += WiFi.SSID(i) + "$";
    }
    
    httpd_resp_send(req, response.c_str(), response.length());

    return ESP_OK;
}

// For configuring the device
// Format  : "<hostname>$<SSID>$<password>$"
// Example : "Living Room$myWiFi$really Strong Password$"
esp_err_t WiFiHandler::http_config_handler(httpd_req_t *req)
{
    WiFiled->blink_once();
    
    char content[MAX_STR_LEN];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size); 
        if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to /ac :%s", content);
    
    std::string response = "Got request";

    httpd_resp_send(req, response.c_str(), response.length());

    config_network(content);

    return ESP_OK;
}

esp_err_t WiFiHandler::config_network(const char* str)
{
    WiFiled->blink_once();
    
    std::string ssid;
    std::string hostname;
    std::string password;

    std::string content;

    content = str;

    int previdx = 0;
    int curridx = 0;

    ESP_LOGI(TAG, "%s", content.c_str());

    curridx = content.find('$', previdx);
    hostname = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;

    curridx = content.find('$', previdx);
    ssid = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;
    
    curridx = content.find('$', previdx);
    password = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;

    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.begin(ssid.c_str(), password.c_str());

    ESP_LOGI(TAG, "%s|%s|%s", hostname.c_str(), ssid.c_str(), password.c_str());

    long now = millis();

    WiFiled->start_blinking();
    
    while(WiFi.status() != WL_CONNECTED && (millis() - now < WIFI_TIMEOUT*1000))
    {   
        if(WiFi.status() == WL_NO_SSID_AVAIL)
        {
            ESP_LOGI(TAG, "Connection failed");
            return ESP_FAIL;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    WiFiled->stop_blinking();

    if(WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected successfully");

        nvs_set_str(nvs_wifi, NVS_HOSTNAME_KEY, hostname.c_str());

        nvs_set_str(nvs_wifi, NVS_SSID_KEY, ssid.c_str());

        nvs_set_str(nvs_wifi, NVS_PASSWORD_KEY, password.c_str());

        nvs_commit(nvs_wifi);
        esp_restart();
        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "Connection failed");
        esp_restart();
        return ESP_FAIL;
    }    
}

esp_err_t WiFiHandler::connect_to_network(const char* ssid ,const char* password)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.begin(ssid, password);

    
    while(WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(1000);
    }
    
    Serial.println("WiFi Setup done. Setting up server");

    return ESP_OK;
}

esp_err_t WiFiHandler::start_mdns(const char* hostname)
{
    Serial.println("Server setup, starting mDNS");

    Serial.println(MDNS.begin(hostname));

    MDNS.addService("http", "tcp", 80);

    MDNS.addServiceTxt("http", "tcp", "test", "100");

    Serial.println("Done");

    return ESP_OK;
}

WiFiHandler::WiFiHandler(LedHandler *wifi, LedHandler *ir, SendHandler *send, ReceiveHandler *recv)
{
    WiFiled     = wifi;
    IRled       = ir;
    sender      = send;
    receiver    = recv;
    
    nvs_flash_init();
    
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_wifi);
    
    mode = !(nvs_get_str(nvs_wifi, NVS_SSID_KEY, NULL, NULL) == ESP_ERR_NVS_NOT_FOUND);

    ESP_LOGI("t", "Setup wifi handler. Wifi %s configured", mode?"is":"not");
}

bool WiFiHandler::is_configured()
{
    return mode;
}

bool WiFiHandler::is_connected()
{
    if(!mode)
        return false;
    else
        return WiFi.isConnected();
}

esp_err_t WiFiHandler::start_configure(const char* ap_ssid, const char* ap_password)
{
    if(mode)
        return ESP_FAIL;

    WiFiled->start_blinking();

    ESP_LOGI(TAG, "Configuration not done. Configuration mode activated");
    
    WiFi.softAP(ap_ssid, ap_password);
    
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

        httpd_uri_t uri_scan;
    uri_scan.handler = &http_scan_handler;
    uri_scan.method  = HTTP_GET;
    uri_scan.uri = HTTP_WIFI_SCAN_URI;
    uri_scan.user_ctx = NULL;

    httpd_uri_t uri_config;
    uri_config.handler = &http_config_handler;
    uri_config.method  = HTTP_POST;
    uri_config.uri = HTTP_WIFI_CONFIG_URI;
    uri_config.user_ctx = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_scan);
        httpd_register_uri_handler(server, &uri_config);        
    }

    WiFiled->stop_blinking();

    return ESP_OK;
}

esp_err_t WiFiHandler::auto_connect()
{   
    if(!mode)
        return ESP_FAIL;

    WiFiled->start_blinking();
    
    char* ssid;
    size_t ssid_len;

    char* password;
    size_t password_len;

    char* hostname;
    size_t hostname_len;

    nvs_get_str(nvs_wifi, NVS_SSID_KEY, NULL, &ssid_len);
    ssid = (char*)malloc(ssid_len);
    nvs_get_str(nvs_wifi, NVS_SSID_KEY, ssid, &ssid_len);

    nvs_get_str(nvs_wifi, NVS_PASSWORD_KEY, NULL, &password_len);
    password = (char*)malloc(password_len);
    nvs_get_str(nvs_wifi, NVS_PASSWORD_KEY, password, &password_len);
        
    nvs_get_str(nvs_wifi, NVS_HOSTNAME_KEY, NULL, &hostname_len);
    hostname = (char*)malloc(hostname_len);
    nvs_get_str(nvs_wifi, NVS_HOSTNAME_KEY, hostname, &hostname_len);

    ESP_LOGI(TAG, "Configuration detected : hostename-%s,SSID-%s,password-%s", hostname, ssid, password);

    connect_to_network(ssid, password);

	httpd_uri_t uri_get;
    uri_get.handler = &http_get_handler;
    uri_get.method  = HTTP_GET;
    uri_get.uri = HTTP_GET_URI;
    uri_get.user_ctx = NULL;

	httpd_uri_t uri_post;
    uri_post.handler = &http_post_handler;
    uri_post.method  = HTTP_POST;
    uri_post.uri = HTTP_RAW_SEND_URI;
    uri_post.user_ctx = NULL;

    httpd_uri_t uri_ac_post;
    uri_ac_post.handler = &http_ac_post_handler;
    uri_ac_post.method  = HTTP_POST;
    uri_ac_post.uri = HTTP_AC_SEND_URI;
    uri_ac_post.user_ctx = NULL;

    httpd_uri_t uri_scan;
    uri_scan.handler = &http_scan_handler;
    uri_scan.method  = HTTP_GET;
    uri_scan.uri = HTTP_WIFI_SCAN_URI;
    uri_scan.user_ctx = NULL;

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the http server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_ac_post);    
        httpd_register_uri_handler(server, &uri_scan);    
    }

    start_mdns(hostname);

    WiFiled->stop_blinking();

    ESP_LOGI("t", "OK");

    return ESP_OK;
}

esp_err_t WiFiHandler::force_connect(const char* ssid, const char* password, const char* hostname)
{
    connect_to_network(ssid, password);

    httpd_uri_t uri_get;
    uri_get.handler = &http_get_handler;
    uri_get.method  = HTTP_GET;
    uri_get.uri = HTTP_GET_URI;
    uri_get.user_ctx = NULL;

	httpd_uri_t uri_post;
    uri_post.handler = &http_post_handler;
    uri_post.method  = HTTP_POST;
    uri_post.uri = HTTP_RAW_SEND_URI;
    uri_post.user_ctx = NULL;

    httpd_uri_t uri_ac_post;
    uri_ac_post.handler = &http_ac_post_handler;
    uri_ac_post.method  = HTTP_POST;
    uri_ac_post.uri = HTTP_AC_SEND_URI;
    uri_ac_post.user_ctx = NULL;

    httpd_uri_t uri_scan;
    uri_scan.handler = &http_scan_handler;
    uri_scan.method  = HTTP_GET;
    uri_scan.uri = HTTP_WIFI_SCAN_URI;
    uri_scan.user_ctx = NULL;

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the http server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_ac_post);    
        httpd_register_uri_handler(server, &uri_scan);    
    }

    start_mdns(hostname);

    return ESP_OK;
}
