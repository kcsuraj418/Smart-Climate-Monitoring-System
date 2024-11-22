#include <Wire.h>
#include "Adafruit_AHTX0.h"  // Library for AHT20/DHT20
#include <Arduino.h>
#include <HttpClient.h>
#include <WiFi.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

// WiFi credentials
char ssid[50]; // Network SSID
char pass[50]; // Network password

// Server details
const char kHostname[] = "54.219.132.150";
const char kPath[] = "/"; // Path to send data
const int kPort = 5000;

// Sensor object
Adafruit_AHTX0 aht;

// Define I2C pins
#define SDA_PIN 33
#define SCL_PIN 32  // Update this with the GPIO pin you'll connect for SCL

// Function to access NVS for WiFi credentials
void nvs_access() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        size_t ssid_len = sizeof(ssid);
        size_t pass_len = sizeof(pass);
        err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        err |= nvs_get_str(my_handle, "pass", pass, &pass_len);
        if (err != ESP_OK) {
            Serial.println("Failed to retrieve WiFi credentials from NVS.");
        }
        nvs_close(my_handle);
    } else {
        Serial.println("Error opening NVS handle.");
    }
}

// WiFi connection setup
void setup() {
    Serial.begin(9600);
    delay(1000);

    nvs_access();  // Retrieve SSID and password
    delay(1000);

    Serial.println();
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize I2C with custom pins
    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize AHT20 sensor
    if (!aht.begin()) {
        Serial.println("Could not find AHT20 sensor. Check wiring!");
        while (1) delay(10);
    }
    Serial.println("AHT20 sensor initialized.");
}

// Loop to read sensor data and send to server
void loop() {
    // Read temperature and humidity
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);  // Read data into the event objects

    float temperature = temp.temperature;
    float hum = humidity.relative_humidity;

    // Validate readings
    if (isnan(hum) || isnan(temperature)) {
        Serial.println("Failed to read from AHT20 sensor!");
        delay(2000);
        return;
    }

    // Print temperature and humidity to the serial monitor
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);  // Print temperature with 1 decimal point
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(hum, 1);  // Print humidity with 1 decimal point
    Serial.println(" %");

    // Format the data as JSON
    String payload = "{";
    payload += "\"temperature\": " + String(temperature, 1) + ",";
    payload += "\"humidity\": " + String(hum, 1);
    payload += "}";

    // Send data to the server
    WiFiClient c;
    HttpClient http(c, kHostname, kPort);  // Corrected constructor

    Serial.println("Sending data to server...");
    int err = http.post(kPath, "application/json", payload.c_str());  // Corrected method
    if (err == 0) {
        int statusCode = http.responseStatusCode();
        Serial.print("Response status code: ");
        Serial.println(statusCode);

        if (statusCode == 200) {
            Serial.println("Data sent successfully.");
        } else {
            Serial.println("Failed to send data.");
        }
    } else {
        Serial.print("HTTP POST failed: ");
        Serial.println(err);
    }

    http.stop();

    delay(10000);  // Wait 10 seconds before sending again
}
