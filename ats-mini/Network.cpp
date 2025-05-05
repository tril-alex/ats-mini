#include "Common.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>

bool wifiInit();
void webInit();
void webSetFreq(AsyncWebServerRequest *request);
void webSetVol(AsyncWebServerRequest *request);
void webConfig(AsyncWebServerRequest *request);
void webConnectWifi(AsyncWebServerRequest *request);
String webRadioData();
String webRadioPage();
String webConfigPage();
String webChartPage(bool fillChart);

static const char *apSSID    = "ATS-Mini";
static const char *apPWD     = 0;           // No password
static const int   apChannel = 10;          // WiFi channel number (1..13)
static const bool  apHideMe  = false;       // TRUE: disable SSID broadcast
static const int   apClients = 3;       // Maximum simultaneous connected clients

static uint16_t ajaxInterval = 2500;

// Settings
int utcOffsetInSeconds  = 0;
String iceCastServerURL = "";
String chartTimeZone    = "";
String loginUsername    = "";
String loginPassword    = "";
String webLog           = "";

// AsyncWebServer object on port 80
AsyncWebServer server(80);

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

bool wifiInit()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPWD, apChannel, apHideMe, apClients);

  IPAddress ip(192, 168, 5, 5);
  IPAddress gateway(192, 168, 5, 1); 
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gateway, subnet);

  tft.print("AP Mode IP : ");
  tft.println(WiFi.softAPIP().toString());

//  preferences.begin("configData", false);

  int wifiCheck = 0;
  for(int j=0 ; (j<3) && (WiFi.status()!=WL_CONNECTED) ; j++)
  {
    const char *ssid, *password;
    char text[16];

    sprintf(text, "ssid%d", j+1);
    ssid = "ssid";//preferences.getString(text, "");
    sprintf(text, "password%d", j+1);
    password = "PASSWORD";//preferences.getString(text, "");

    if(*ssid)
    {
      WiFi.begin(ssid, password);
      tft.print("Connecting WiFi");
      while((WiFi.status()!=WL_CONNECTED) && (wifiCheck<30))
      {
        if(!(wifiCheck&7)) tft.print(".");
        wifiCheck++;
        delay(500);
        if(digitalRead(ENCODER_PUSH_BUTTON)==LOW)
        {
          WiFi.disconnect();
          wifiCheck = 30;
        }
      }
    }
  }

//  iceCastServerURL   = preferences.getString("icecastserverurl", "");
//  utcOffsetInSeconds = preferences.getString("utcoffset", "").toInt();
//  chartTimeZone      = preferences.getString("charttimezone", "");
//  loginUsername      = preferences.getString("loginusername", "");
//  loginPassword      = preferences.getString("loginpassword", "");
//  preferences.end();

  if(WiFi.status()!=WL_CONNECTED)
  {
    tft.println(" No WiFi connection");
    ajaxInterval = 2500;
  }
  else
  {
    tft.println("Connected ");
    tft.print("WiFi IP : ");
    tft.print(WiFi.localIP().toString());

    if(utcOffsetInSeconds)
    {
      ntpClient.setTimeOffset(utcOffsetInSeconds);
      ntpClient.update();      
    }
    
    ajaxInterval = 1000;
  }

  // Done
  delay(WiFi.status()==WL_CONNECTED? 2000 : 1000);
  return(WiFi.status()==WL_CONNECTED);
}

void webInit()
{
  server.on("/", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    String page = webRadioPage();

    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "text/html", [page] (uint8_t *buffer, size_t maxlen, size_t index) -> size_t {
        size_t len = min(maxlen, page.length() - index);
        memcpy(buffer, page.c_str() + index, len);
        return len;
      }
    );

    request->send(response);
  });

  server.on("/data", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", webRadioData());
  });
  
  server.on("/configpage", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(loginUsername.c_str(), loginPassword.c_str()))
      return request->requestAuthentication();
    request->send(200, "text/html", webConfigPage());
  });

  server.on("/rssi", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    rx.getCurrentReceivedSignalQuality();
    request->send(200, "text/plain", String(rx.getCurrentRSSI()));
  });

  server.on("/snr", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    rx.getCurrentReceivedSignalQuality();
    request->send(200, "text/plain", String(rx.getCurrentSNR()));
  });

  server.on("/chart", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    bool fillChart = request->hasParam("fillChart", true) && request->getParam("fillChart", true)->value() == "fill";
    request->send(200, "text/html", webChartPage(fillChart));
  });

  server.onNotFound([] (AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.on("/logout", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(401);
  });

//  server.on("/logged-out", HTTP_GET, [] (AsyncWebServerRequest *request) {
//    request->send(200, "text/html", logout_html);
//  });

  server.on("/weblog", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", webLog);
  });

  server.on("/setfrequency", HTTP_ANY, webSetFreq);
  server.on("/setvolume",    HTTP_ANY, webSetVol);
  server.on("/config",       HTTP_ANY, webConfig);
  server.on("/connectwifi",  HTTP_ANY, webConnectWifi);

  // Start web server  
  server.begin();
}

void webSetFreq(AsyncWebServerRequest *request)
{
  request->send(200);
}

void webSetVol(AsyncWebServerRequest *request)
{
  request->send(200);
}

void webConfig(AsyncWebServerRequest *request)
{
  request->send(200);
}

void webConnectWifi(AsyncWebServerRequest *request)
{
  request->send(200);
}

String webRadioData()
{
  return "";
}

String webRadioPage()
{
  return "";
}

String webConfigPage()
{
  return "";
}

String webChartPage(bool fillChart)
{
  return "";
}
  