#ifndef main_h
#define main_h
#include <Arduino.h>


#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


void getJsonData();
void callback(char *top, byte *payload, unsigned int length);
bool sendtoMQTT();
void clearJsonData();
static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

#endif  // main_h