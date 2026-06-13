void initWiFi() {
  Serial.print("WiFi");
  WiFi.onEvent(WiFiEvent);
  if (WiFiMode == AP_MODE) {
    Serial.println("_AP");
    WiFiSetMode(AP_MODE);
  } else {
    Serial.println("_CLIENT");
    WiFiSetMode(WiFiMode);
  }
}

void WiFiEvent(WiFiEvent_t event){
    switch(event) {
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi AP Started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            break;
        case SYSTEM_EVENT_STA_START:
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("WiFi STA Got IP");
            break;
        default:
            break;
    }
}

void WiFiSetMode(int setWiFiMode) {
  WiFiMode = settingsSaveWiFi(setWiFiMode);
  if (WiFiMode == AP_MODE) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifiSsid[WiFiMode], wifiPass[WiFiMode]);
  } else {
    WiFi.setHostname(wifiSsid[WiFiMode]);
    WiFi.begin(wifiSsid[WiFiMode], wifiPass[WiFiMode]);
  }
}

void menuWiFiSetMode(int setWiFiMode) {
  WiFiSetMode(setWiFiMode);
}

void updateWiFi() {
  if (WiFiMode == AP_MODE) {
    WiFiIP = WiFi.softAPIP();
  } else {
    WiFiIP = WiFi.localIP();
  }
}

double WiFiInfo(double info) {
  cliSerial->print("IP: ");
  cliSerial->println(WiFiIP);
  cliSerial->print("SSID: ");
  cliSerial->println(wifiSsid[WiFiMode]);
    
  if (WiFiMode == AP_MODE) {
    cliSerial->print("Pass: ");
    cliSerial->println(wifiPass[WiFiMode]);
  }

  return 1;
}
