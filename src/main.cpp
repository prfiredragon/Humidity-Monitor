#include <Arduino.h>
#include <capacitive_soil_moisture_sensor.h>

#include <main.h>



#include <EEPROM.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>

#include "Settings.h"

#include "webpages/htmlCase.h"     // The HTML Konstructor
#include "webpages/main.h"         // landing page with menu
#include "webpages/settings.h"     // settings page
#include "webpages/settingsedit.h" // mqtt settings page

WiFiClient client;
Settings _settings;
PubSubClient mqttclient(client);
int jsonBufferSize = 2048;
char jsonBuffer[2048];

int requestTime;

capacitive_soil_moisture_sensor csms;

DynamicJsonDocument ctrlJson(jsonBufferSize);                      // main Json
JsonObject GenJson = ctrlJson.createNestedObject("Pack");         // battery package data




String topicStrg;

unsigned long mqtttimer = 0;
unsigned long bmstimer = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;




// flag for saving data and other things
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool updateProgress = false;
bool dataCollect = false;
int crcErrCount = 0;
bool firstPublish = false;

//----------------------------------------------------------------------
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("Update");
#endif
    Update.runAsync(true);
    if (!Update.begin(free_space))
    {
#ifdef DALY_BMS_DEBUG
      Update.printError(DALY_BMS_DEBUG);
#endif
    }
  }

  if (Update.write(data, len) != len)
  {
#ifdef DALY_BMS_DEBUG
    Update.printError(DALY_BMS_DEBUG);
#endif
  }

  if (final)
  {
    if (!Update.end(true))
    {
#ifdef DALY_BMS_DEBUG
      Update.printError(DALY_BMS_DEBUG);
#endif
    }
    else
    {

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device is booting new Firmware");
      response->addHeader("Refresh", "10; url=/");
      response->addHeader("Connection", "close");
      request->send(response);

      restartNow = true; // Set flag so main loop can issue restart call
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("Update complete");
#endif
    }
  }
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend())
  {
    //serializeJson(bmsJson, jsonBuffer);
    serializeJson(ctrlJson, jsonBuffer);
    wsClient->text(jsonBuffer);
    // ws.textAll(jsonBuffer);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    updateProgress = true;
    if (strcmp((char *)data, "dischargeFetSwitch_on") == 0)
    {
      //bms.setDischargeMOS(true);
    }
    if (strcmp((char *)data, "dischargeFetSwitch_off") == 0)
    {
      //bms.setDischargeMOS(false);
    }
    if (strcmp((char *)data, "chargeFetSwitch_on") == 0)
    {
      //bms.setChargeMOS(true);
    }
    if (strcmp((char *)data, "chargeFetSwitch_off") == 0)
    {
      //bms.setChargeMOS(false);
    }
    delay(200); // give the bms time to react
    updateProgress = false;
    /*
     if (strcmp((char *)data, "dataRequired") == 0)
     {
       //bmstimer = -3000; // set the timer to zero to get instant data to web
       // notifyClients();
     }
     */
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsClient = client;
    //gbmb.update();
    getJsonData();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    wsClient = nullptr;
    //  Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
 // wifi_set_sleep_type(LIGHT_SLEEP_T); // for testing

  _settings.load();
  delay(1000);                                      // wait for what?
  //gbmb.Init();                                       // init the Gardener Base driver
  WiFi.persistent(true);                           // fix wifi save bug
  //packJson["Device_Name"] = _settings._deviceName; // set the device name in json string
  GenJson["Device_Name"] = _settings._deviceName; // set the device name in json string
  //topic = _settings._mqttTopic;
  topicStrg = (_settings._mqttTopic/*topic*/ + "/" + _settings._deviceName).c_str(); // new test for simplify mqtt publishes
  AsyncWiFiManager wm(&server, &dns);
  bmstimer = millis();
  mqtttimer = millis();

  csms.get.dry_val = _settings._default_dry_val;
  csms.get.wet_val = _settings._default_wet_val;

  wm.setSaveConfigCallback(saveConfigCallback);

  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", NULL, 6);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", NULL, 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", NULL, 32);
  AsyncWiFiManagerParameter custom_default_dry_val("default_dry_val", "Dry Val", NULL, 6);
  AsyncWiFiManagerParameter custom_default_wet_val("default_wet_val", "Wet Val", NULL, 6);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);
  wm.addParameter(&custom_default_dry_val);
  wm.addParameter(&custom_default_wet_val);

  bool res = wm.autoConnect("HUMIDITY-SENSOR-AP");

  wm.setConnectTimeout(30);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds

  // save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
    _settings._mqttServer = custom_mqtt_server.getValue();
    _settings._mqttUser = custom_mqtt_user.getValue();
    _settings._mqttPassword = custom_mqtt_pass.getValue();
    _settings._mqttPort = atoi(custom_mqtt_port.getValue());
    _settings._deviceName = custom_device_name.getValue();
    _settings._mqttTopic = custom_mqtt_topic.getValue();
    _settings._mqttRefresh = atoi(custom_mqtt_refresh.getValue());
    _settings._default_dry_val = atoi(custom_default_dry_val.getValue());
    _settings._default_wet_val = atoi(custom_default_wet_val.getValue());

    _settings.save();
    delay(500);
    //_settings.load();
    ESP.restart();
  }

  
  mqttclient.setServer(_settings._mqttServer.c_str(), _settings._mqttPort);
  mqttclient.setCallback(callback);
  mqttclient.setBufferSize(jsonBufferSize);
  // check is WiFi connected
  if (!res)
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("Failed to connect or hit timeout");
#endif
  }
  else
  {


    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_MAIN);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                serializeJson(ctrlJson, *response);
                request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots...");
                response->addHeader("Refresh", "5; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                restartNow = true; });

    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_CONFIRM_RESET);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                delay(1000);
                _settings.reset();
                ESP.eraseConfig();
                ESP.restart(); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS_EDIT);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsjson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument SettingsJson(256);
                SettingsJson["device_name"] = _settings._deviceName;
                SettingsJson["mqtt_server"] = _settings._mqttServer;
                SettingsJson["mqtt_port"] = _settings._mqttPort;
                SettingsJson["mqtt_topic"] = _settings._mqttTopic;
                SettingsJson["mqtt_user"] = _settings._mqttUser;
                SettingsJson["mqtt_password"] = _settings._mqttPassword;
                SettingsJson["mqtt_refresh"] = _settings._mqttRefresh;
                SettingsJson["default_dry_val"] = _settings._default_dry_val;
                SettingsJson["default_wet_val"] = _settings._default_wet_val;
                SettingsJson["mqtt_json"] = _settings._mqttJson?true:false;
                serializeJson(SettingsJson, *response);
                request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                request->redirect("/settings");
                _settings._mqttServer = request->arg("post_mqttServer");
                _settings._mqttPort = request->arg("post_mqttPort").toInt();
                _settings._mqttUser = request->arg("post_mqttUser");
                _settings._mqttPassword = request->arg("post_mqttPassword");
                _settings._mqttTopic = request->arg("post_mqttTopic");
                _settings._mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 :  request->arg("post_mqttRefresh").toInt(); //prefent lower numbers
                _settings._deviceName = request->arg("post_deviceName");
                _settings._default_dry_val = request->arg("post_default_dry_val").toInt();
                _settings._default_wet_val = request->arg("post_default_wet_val").toInt();
                if(request->arg("post_mqttjson") == "true") _settings._mqttJson = true;
                if(request->arg("post_mqttjson") != "true") _settings._mqttJson = false;
                Serial.print(_settings._mqttServer);
                _settings.save();
                //delay(500);
                //_settings.load();
                });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "chargefet")
                {
#ifdef DALY_BMS_DEBUG
                    DALY_BMS_DEBUG.println("charge fet webswitch to: "+(String)p->value());
#endif
                    if(p->value().toInt() == 1){
                      //bms.setChargeMOS(true);
                      //bms.get.chargeFetState = true;
                    }
                    if(p->value().toInt() == 0){
                      //bms.setChargeMOS(false);
                      //bms.get.chargeFetState = false;
                    }
                }
                if (p->name() == "dischargefet")
                {
#ifdef DALY_BMS_DEBUG
                    DALY_BMS_DEBUG.println("discharge fet webswitch to: "+(String)p->value());
#endif
                    if(p->value().toInt() == 1){
                      //bms.setDischargeMOS(true);
                      //bms.get.disChargeFetState = true;
                    }
                    if(p->value().toInt() == 0){
                      //bms.setDischargeMOS(false);
                      //bms.get.disChargeFetState = false;
                    }
                }
                request->send(200, "text/plain", "message received"); });

    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          updateProgress = true;
          //delay(500);
          request->send(200);
          request->redirect("/"); },
        handle_update_progress_cb);
        
    // set the device name
    MDNS.begin(_settings._deviceName);
    WiFi.hostname(_settings._deviceName);
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
    MDNS.addService("http", "tcp", 80);
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("Webserver Running...");
#endif
  }

  if (!mqttclient.connected())
    mqttclient.connect((String(_settings._deviceName)).c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str());
  if (mqttclient.connect(_settings._deviceName.c_str()))
  {
    if (!_settings._mqttJson)
    {
      mqttclient.subscribe((topicStrg + "/wet_val").c_str());
      mqttclient.subscribe((topicStrg + "/dry_val").c_str());
    }
    else
    {
      mqttclient.subscribe((topicStrg + "/" + _settings._deviceName).c_str());
    }
  }
}
// end void setup

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void loop()
{

  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  {                      // No use going to next step unless WIFI is up and running.
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();
    mqttclient.loop(); // Check if we have something to read from MQTT
    //gbmb.update();

    if (!updateProgress)
    {
      bool updatedData = false;
      if (millis() > (bmstimer + (1 * 1000)) && wsClient != nullptr && wsClient->canSend())
      {
        bmstimer = millis();
        //if (gbmb.update()) // ask the bms for new data
        //{
          getJsonData();
          crcErrCount = 0;
          updatedData = true;
        /*}
        else
        {
          crcErrCount++;
          if (crcErrCount >= 3)
          {
            clearJsonData(); // by no connection, clear all data
            updatedData = false;
          }
        }
        */
        notifyClients();
      }
      else if (millis() > (mqtttimer + (_settings._mqttRefresh * 100)))
      {
        mqtttimer = millis();
        if (millis() < (bmstimer + (1 * 1000)) && updatedData == true) // if the last request shorter then 3 use the data from last web request
        {
          sendtoMQTT(); // Update data to MQTT server if we should
        }
        else // get new data
        {
          requestTime = millis();
          //if (gbmb.update()) // ask the bms for new data
          //{
            getJsonData(); //prepare data for json string sending
            sendtoMQTT(); // Update data to MQTT server if we should
            crcErrCount = 0;
            updatedData = true;
          /*}
          else
          {
            crcErrCount++;
            if (crcErrCount >= 3)
            {
              clearJsonData(); // by no connection, clear all data
              updatedData = false;
              
            }
          }
          */
        }
      }
    }
    if (wsClient == nullptr)
    {
     // delay(2);
    }
  }
  if (restartNow)
  {
    Serial.println("Restart");
    ESP.restart();
  }
  yield();
}
// End void loop
void getJsonData()
{
  // prevent buffer leak
  if (int(ctrlJson.memoryUsage()) >= (jsonBufferSize - 8))
  {
    ctrlJson.garbageCollect();
  }


  GenJson["Device_IP"] = WiFi.localIP().toString();  
  GenJson["Value"] = csms.value();
  GenJson["percentage"] = csms.percentage();
  GenJson["wet_val"] = csms.get.wet_val;

  GenJson["dry_val"] = csms.get.dry_val;

  
}

void clearJsonData()
{
  GenJson["Value"] = nullptr;
  GenJson["percentage"] = nullptr;
  GenJson["wet_val"] = nullptr;
  GenJson["dry_val"] = nullptr;

}

bool sendtoMQTT()
{
  if (!mqttclient.connected())
  {
    _settings.load(); //fix w
    if (mqttclient.connect(((_settings._deviceName)).c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str()))
    {
      if (!_settings._mqttJson)
      {
        mqttclient.publish((topicStrg + ("/Device_IP")).c_str(), (WiFi.localIP().toString()).c_str());
      }
    }
    else
    {
      return false; // Exit if we couldnt connect to MQTT brooker
    }
  }
  
  if (!_settings._mqttJson)
  {

    //int mqttRuntime = millis();
    char msgBuffer[20];

    
    mqttclient.publish((topicStrg + "/Value").c_str(), dtostrf(csms.value(), 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/percentage").c_str(), dtostrf(csms.percentage(), 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/wet_val").c_str(), dtostrf(csms.get.wet_val, 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/dry_val").c_str(), dtostrf(csms.get.dry_val, 4, 1, msgBuffer));;

  }
  else
  {
    size_t n = serializeJson(ctrlJson, jsonBuffer);
    mqttclient.publish((String(topicStrg)).c_str(), jsonBuffer, n);
  }
  firstPublish = true;
  return true;
}

void callback(char *top, byte *payload, unsigned int length)
{
  if(firstPublish == false) return;
  updateProgress = true;
  if (!_settings._mqttJson)
  {
    String messageTemp;
    for (unsigned int i = 0; i < length; i++)
    {
      messageTemp += (char)payload[i];
    }
    
    // 
    if (strcmp(top, (topicStrg + "/wet_val").c_str()) == 0)
    {
        csms.get.wet_val = messageTemp.toInt();
    }

    // 
    if (strcmp(top, (topicStrg + "/dry_val").c_str()) == 0)
    {
        csms.get.dry_val = messageTemp.toInt();
      
    }
  }
  else
  {
    StaticJsonDocument<2048> mqttJsonAnswer;
    deserializeJson(mqttJsonAnswer, (const byte *)payload, length);

      csms.get.wet_val = mqttJsonAnswer["Pack"]["wet_val"];
      csms.get.dry_val = mqttJsonAnswer["Pack"]["dry_val"];

    
  }
  updateProgress = false;
}