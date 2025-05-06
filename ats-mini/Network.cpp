#include "Common.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <Preferences.h>

static void webSetFreq(AsyncWebServerRequest *request);
static void webSetVol(AsyncWebServerRequest *request);
static void webSetConfig(AsyncWebServerRequest *request);
static void webConnectWifi(AsyncWebServerRequest *request);
static const String webRadioData();
static const String webRadioPage();
static const String webConfigPage();
static const String webChartPage(bool fillChart);

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

// Network preferences saved here
Preferences preferences;

// AsyncWebServer object on port 80
AsyncWebServer server(80);

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

bool ntpSyncTime()
{
  if(WiFi.status()==WL_CONNECTED)
  {
    ntpClient.setTimeOffset(utcOffsetInSeconds);
    ntpClient.update();

    if(ntpClient.isTimeSet())
      return(clockSet(ntpClient.getHours(), ntpClient.getMinutes()));
  }

  return(false);
}

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

  preferences.begin("configData", false);

  int wifiCheck = 0;
  for(int j=0 ; (j<3) && (WiFi.status()!=WL_CONNECTED) ; j++)
  {
    char nameSSID[16], namePASS[16];
    sprintf(nameSSID, "wifissid%d", j+1);
    sprintf(namePASS, "wifipass%d", j+1);

    String ssid = preferences.getString(nameSSID, "");
    String password = preferences.getString(namePASS, "");

    if(ssid != "")
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

  iceCastServerURL   = preferences.getString("icecastserverurl", "");
  utcOffsetInSeconds = preferences.getString("utcoffset", "").toInt();
  chartTimeZone      = preferences.getString("charttimezone", "");
  loginUsername      = preferences.getString("loginusername", "");
  loginPassword      = preferences.getString("loginpassword", "");
  preferences.end();

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
  
  server.on("/config", HTTP_ANY, [] (AsyncWebServerRequest *request) {
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
  server.on("/setconfig",    HTTP_ANY, webSetConfig);
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

void webSetConfig(AsyncWebServerRequest *request)
{
  if(request->hasParam("username", true) && request->getParam("username", true)->value() != "")
  {
    loginUsername = request->getParam("username", true)->value();
    loginPassword = request->getParam("password", true)->value();

    preferences.begin("configData", false);
    preferences.putString("loginusername", loginUsername); 
    preferences.putString("loginpassword", loginPassword);
    preferences.end();
  }

  for(int j=0 ; j<3 ; j++)
  {
    char nameSSID[16], namePASS[16];

    sprintf(nameSSID, "wifissid%d", j+1);
    sprintf(namePASS, "wifipass%d", j+1);

    if(request->hasParam(nameSSID, true) && request->getParam(nameSSID, true)->value() != "")
    {
      preferences.begin("configData", false);
      preferences.putString(nameSSID, request->getParam(nameSSID, true)->value()); 
      preferences.putString(namePASS, request->getParam(namePASS, true)->value());
      preferences.end();
    }
  }
    
  request->send(200);
}

void webConnectWifi(AsyncWebServerRequest *request)
{
  request->send(200);
}

const String webRadioData()
{
  return "";
}

const String webRadioPage()
{
  return
    "<!DOCTYPE HTML>"
    "<HTML>"
    "<HEAD>"
      "<TITLE>ATS-Mini Pocket Receiver</TITLE>"
    "</HEAD>"
    "<BODY>"
      "<H1>ATS-Mini Pocket Receiver</H1>"
    "</BODY>"
    "</HTML>"
    ;
}

const String webConfigPage()
{
  preferences.begin("configData", false);
  String ssid1 = preferences.getString("wifissid1", "");
  String pass1 = preferences.getString("wifipass1", "");
  String ssid2 = preferences.getString("wifissid2", "");
  String pass2 = preferences.getString("wifipass2", "");
  String ssid3 = preferences.getString("wifissid3", "");
  String pass3 = preferences.getString("wifipass3", "");
  preferences.end();

  return
    "<!DOCTYPE HTML>"
    "<HTML>"
    "<HEAD>"
      "<TITLE>ATS-Mini Config</TITLE>"
    "</HEAD>"
    "<BODY>"
      "<H1>ATS-Mini Config</H1>"
      "<FORM ACTION='/setconfig' METHOD='POST'>"
        "<DIV>"
          "<LABEL>Username : </LABEL><INPUT TYPE='TEXT' NAME='username' value='" + loginUsername + "'>"
          "<LABEL>Password : </LABEL><INPUT TYPE='PASSWORD' NAME='password' value='" + loginPassword + "'>"
        "</DIV>"
        "<DIV>"
          "<LABEL>WiFi SSID 1 : </LABEL><INPUT TYPE='TEXT' NAME='wifissid1' value='" + ssid1 + "'>"
          "<LABEL>WiFi PASS 1 : </LABEL><INPUT TYPE='TEXT' NAME='wifipass1' value='" + pass1 + "'>"
        "</DIV>"
        "<DIV>"
          "<LABEL>WiFi SSID 2 : </LABEL><INPUT TYPE='TEXT' NAME='wifissid2' value='" + ssid2 + "'>"
          "<LABEL>WiFi PASS 2 : </LABEL><INPUT TYPE='TEXT' NAME='wifipass2' value='" + pass2 + "'>"
        "</DIV>"
        "<DIV>"
          "<LABEL>WiFi SSID 3 : </LABEL><INPUT TYPE='TEXT' NAME='wifissid3' value='" + ssid3 + "'>"
          "<LABEL>WiFi PASS 3 : </LABEL><INPUT TYPE='TEXT' NAME='wifipass3' value='" + pass3 + "'>"
        "</DIV>"
        "<INPUT TYPE='SUBMIT' value='Save Config'>"
      "</FORM>"
    "</BODY>"
    "</HTML>"
    ;
}

const String webChartPage(bool fillChart)
{
  return "";
}
  