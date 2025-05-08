#include "Common.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <Preferences.h>

//
// Access Point (AP) mode settings
//
static const char *apSSID    = "ATS-Mini";
static const char *apPWD     = 0;       // No password
static const int   apChannel = 10;      // WiFi channel number (1..13)
static const bool  apHideMe  = false;   // TRUE: disable SSID broadcast
static const int   apClients = 3;       // Maximum simultaneous connected clients

static uint16_t ajaxInterval = 2500;

// Settings
int utcOffsetInSeconds = 0;
String loginUsername   = "";
String loginPassword   = "";

// Network preferences saved here
Preferences preferences;

// AsyncWebServer object on port 80
AsyncWebServer server(80);

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

static bool wifiInit(bool connectToNetwork = true);
static void webInit();
static void webSetFreq(AsyncWebServerRequest *request);
static void webSetVol(AsyncWebServerRequest *request);
static void webSetConfig(AsyncWebServerRequest *request);
static void webConnectWifi(AsyncWebServerRequest *request);
static const String webRadioData();
static const String webRadioPage();
static const String webConfigPage();

//
// Stop WiFi hardware
//
void netStop()
{
  WiFi.disconnect(true);
}

//
// Initialize WiFi network and services
//
void netInit(uint8_t netMode)
{
  // Do not initialize WiFi if disabled
  if(netMode==NET_OFF)
  {
    netStop();
    return;
  }

  // Initialize WiFi and try connecting to a network
  if(wifiInit(netMode!=NET_AP_ONLY))
  {
    // Get NTP time from the network
    clockReset();
    ntpSyncTime();
  }

  // If only connected to sync...
  if(netMode==NET_SYNC)
  {
    // Drop network connection
    netStop();
  }
  else
  {
    // Initialize web server for remote configuration
    webInit();
  }
}

//
// Returns TRUE if NTP time is available
//
bool ntpIsAvailable()
{
  return(ntpClient.isTimeSet());
}

//
// Update NTP time and synchronize clock with NTP time
//
bool ntpSyncTime()
{
  if(WiFi.status()==WL_CONNECTED) ntpClient.update();

  if(ntpClient.isTimeSet())
    return(clockSet(ntpClient.getHours(), ntpClient.getMinutes()));
  else
    return(false);
}

//
// Initialize WiFi, connect to an external access point if possible
//
bool wifiInit(bool connectToNetwork)
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPWD, apChannel, apHideMe, apClients);

  IPAddress ip(192, 168, 5, 5);
  IPAddress gateway(192, 168, 5, 1); 
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gateway, subnet);

  String status1 = "AP Mode IP : " + WiFi.softAPIP().toString();
  String status2 = "Connecting to WiFi network..";
  drawWiFiStatus(status1.c_str(), 0);

  // Get the preferences
  preferences.begin("configData", false);
  utcOffsetInSeconds = preferences.getString("utcoffset", "").toInt();
  loginUsername      = preferences.getString("loginusername", "");
  loginPassword      = preferences.getString("loginpassword", "");

  // If not connecting to the network, remain AP and stop here
  if(!connectToNetwork)
  {
    preferences.end();
    ajaxInterval = 2500;
    delay(1000);
    return(false);
  }

  // Try connecting to known WiFi networks
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
      while((WiFi.status()!=WL_CONNECTED) && (wifiCheck<30))
      {
        if(!(wifiCheck&7))
        {
          status2 += ".";
          drawWiFiStatus(status1.c_str(), status2.c_str());
        }
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

  // Done with preferences
  preferences.end();

  if(WiFi.status()!=WL_CONNECTED)
  {
    // WiFi connection failed
    drawWiFiStatus(status1.c_str(), "No WiFi connection");
    // Done
    ajaxInterval = 2500;
    delay(1000);
    return(false);
  }
  else
  {
    // WiFi connection succeeded
    status2 = "IP : " + WiFi.localIP().toString();
    drawWiFiStatus("Connected to WiFi network", status2.c_str());
    // Initiate NTP time updates to happen every 5 minutes
    ntpClient.setTimeOffset(utcOffsetInSeconds);
    ntpClient.setUpdateInterval(5*60*1000);
    ntpClient.update();
    clockReset();
    // Done
    ajaxInterval = 1000;
    delay(2000);
    return(true);
  }
}

//
// Initialize internal web server
//
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
    if(loginUsername != "" && loginPassword != "")
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

  server.onNotFound([] (AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.on("/logout", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(401);
  });

//  server.on("/logged-out", HTTP_GET, [] (AsyncWebServerRequest *request) {
//    request->send(200, "text/html", logout_html);
//  });

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
  preferences.begin("configData", false);

  if(request->hasParam("username", true) && request->getParam("username", true)->value() != "")
  {
    loginUsername = request->getParam("username", true)->value();
    loginPassword = request->getParam("password", true)->value();

    preferences.putString("loginusername", loginUsername); 
    preferences.putString("loginpassword", loginPassword);
  }

  for(int j=0 ; j<3 ; j++)
  {
    char nameSSID[16], namePASS[16];

    sprintf(nameSSID, "wifissid%d", j+1);
    sprintf(namePASS, "wifipass%d", j+1);

    if(request->hasParam(nameSSID, true) && request->getParam(nameSSID, true)->value() != "")
    {
      preferences.putString(nameSSID, request->getParam(nameSSID, true)->value()); 
      preferences.putString(namePASS, request->getParam(namePASS, true)->value());
    }
  }

  if(request->hasParam("utcoffset", true) && request->getParam("utcoffset", true)->value() != "")
  {
    String utcOffset = request->getParam("utcoffset", true)->value();
    preferences.putString("utcoffset", utcOffset); 
    utcOffsetInSeconds = utcOffset.toInt();
    ntpClient.setTimeOffset(utcOffsetInSeconds);
    clockReset();
  }
  
  preferences.end();
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
      "<META charset='UTF-8'>"
      "<META name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<TITLE>ATS-Mini Pocket Receiver</TITLE>"
    "</HEAD>"
    "<BODY>"
      "<H1>ATS-Mini Pocket Receiver</H1>"
    "</BODY>"
    "</HTML>"
    ;
}

static const String webUtcOffsetSelector()
{
  String result = "";

  for(int j=-5 ; j<=5 ; j++)
  {
    int offset = j * 3600;
    char text[64];

    sprintf(text,
      "<OPTION VALUE='%d'%s>UTC%+d</OPTION>",
      offset, offset==utcOffsetInSeconds? " SELECTED":"", j
    );

    result += text;
  }

  return(result);
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
  "<META charset='UTF-8'>"
  "<META name='viewport' content='width=device-width, initial-scale=1.0'>"
  "<TITLE>ATS-Mini Config</TITLE>"
  "<STYLE>"
    "H1"
    "{"
      "text-align: center;"
    "}"
    "TABLE"
    "{"
      "width: 100%;"
      "max-width: 768px;"
      "border: 0px;"
      "margin-left: auto;"
      "margin-right: auto;"
    "}"
    "TH, TD"
    "{"
      "padding: 5px;"
    "}"
    "TH.HEADING"
    "{"
      "background-color: #80A0FF;"
      "column-span: all;"
      "text-align: center;"
    "}"
    "TD.LABEL"
    "{"
      "text-align: right;"
    "}"
    "INPUT[type=submit]"
    "{"
      "width: 50%;"
      "padding: 0.5em 0;"
    "}"
  "</STYLE>"
"</HEAD>"
"<BODY STYLE='font-family: sans-serif;'>"
  "<H1>ATS-Mini Config</H1>"
  "<FORM ACTION='/setconfig' METHOD='POST'>"
    "<TABLE COLUMNS=2>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>Login Credentials</TH></TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Username</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='username' VALUE='" + loginUsername + "'></TD>"
    "</TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Password</TD>"
      "<TD><INPUT TYPE='PASSWORD' NAME='password' VALUE='" + loginPassword + "'></TD>"
    "</TR>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>WiFi Network 1</TH></TR>"
    "<TR>"
      "<TD CLASS='LABEL'>SSID</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifissid1' VALUE='" + ssid1 + "'></TD>"
    "</TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Password</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifipass1' VALUE='" + pass1 + "'></TD>"
    "</TR>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>WiFi Network 2</TH></TR>"
    "<TR>"
      "<TD CLASS='LABEL'>SSID</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifissid2' VALUE='" + ssid2 + "'></TD>"
    "</TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Password</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifipass2' VALUE='" + pass2 + "'></TD>"
    "</TR>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>WiFi Network 3</TH></TR>"
    "<TR>"
      "<TD CLASS='LABEL'>SSID</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifissid3' VALUE='" + ssid3 + "'></TD>"
    "</TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Password</TD>"
      "<TD><INPUT TYPE='TEXT' NAME='wifipass3' VALUE='" + pass3 + "'></TD>"
    "</TR>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>Settings</TH></TR>"
    "<TR>"
      "<TD CLASS='LABEL'>Time Zone</TD>"
      "<TD>"
        "<SELECT STYLE='width: 100%;' NAME='utcoffset'>" + webUtcOffsetSelector() + "</SELECT>"
      "</TD>"
    "</TR>"
    "<TR><TH COLSPAN=2 CLASS='HEADING'>"
      "<INPUT TYPE='SUBMIT' VALUE='Save'>"
    "</TH></TR>"
    "</TABLE>"
  "</FORM>"
"</BODY>"
"</HTML>"
;
}
