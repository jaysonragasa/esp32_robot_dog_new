extern uint16_t activeServoMin;
extern uint16_t activeServoMax;
extern double activeMaxAngleRad;

void initWebServer() {
  Serial.print("WebServer ");
  initWebServerRoutes();
  Serial.println();
}

void initWebServerRoutes() {  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html",  index_html_gz, index_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);  
  });
    
  // Dinamic config
  server.on("/c.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/x-javascript", "var c={w:'ws://" + WiFiIP.toString() + "/ws'}");
  });

  server.on("/calib", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();
      if (action == "start") {
        HALEnabled = false;
        isCalibrating = true;
        setServoToCalibrate();
        request->send(200, "text/plain", "Calib started");
      } else if (action == "stop") {
        HALEnabled = true;
        isCalibrating = false;
        request->send(200, "text/plain", "Calib stopped");
      } else if (action == "set") {
        if (request->hasParam("leg") && request->hasParam("joint") && request->hasParam("val")) {
          int legId = request->getParam("leg")->value().toInt();
          int jointId = request->getParam("joint")->value().toInt();
          double val = request->getParam("val")->value().toDouble();
          if (legId >= 0 && legId < 4 && jointId >= 0 && jointId < 3) {
            setHALTrim(legs[legId], jointId, val);
            setLegPWM(legs[legId]); // Update immediately
            request->send(200, "text/plain", "Set OK");
          } else {
            request->send(400, "text/plain", "Invalid params");
          }
        } else {
          request->send(400, "text/plain", "Missing params");
        }
      } else if (action == "pwm") {
        if (request->hasParam("min") && request->hasParam("max") && request->hasParam("angle")) {
          activeServoMin = request->getParam("min")->value().toInt();
          activeServoMax = request->getParam("max")->value().toInt();
          activeMaxAngleRad = degToRad(request->getParam("angle")->value().toDouble());
          if (isCalibrating) {
            setServoToCalibrate(); // refresh pulse widths immediately
          }
          request->send(200, "text/plain", "PWM updated");
        } else {
          request->send(400, "text/plain", "Missing min/max/angle");
        }
      } else if (action == "get") {
        String json = "{";
        json += "\"pwmMin\":" + String(activeServoMin) + ",";
        json += "\"pwmMax\":" + String(activeServoMax) + ",";
        json += "\"maxAngle\":" + String(radToDeg(activeMaxAngleRad)) + ",";
        for (int i = 0; i < LEG_NUM; i++) {
          json += "\"" + String(i) + "\":[";
          json += String(radToDeg(getHALTrim(legs[i], ALPHA)), 2) + ",";
          json += String(radToDeg(getHALTrim(legs[i], BETA)), 2) + ",";
          json += String(radToDeg(getHALTrim(legs[i], GAMMA)), 2);
          json += "]";
          if (i < LEG_NUM - 1) json += ",";
        }
        json += "}";
        request->send(200, "application/json", json);
      } else {
        request->send(400, "text/plain", "Unknown action");
      }
    } else {
      request->send(400, "text/plain", "Missing action");
    }
  });

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
 
  server.begin();
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
 
  if(type == WS_EVT_CONNECT){ 
    client->text("Ok");
    clientOnline = true;
  } else if (clientOnline && type == WS_EVT_DATA) {
    FS_WS_count = 0;  // zero FS counter

    switch(data[0]) {
      case P_MOVE:
        pMove(data);
        break;
      case P_TELEMETRY:
        pTelemetry();
        client->binary(telemetryPackage, P_TELEMETRY_LEN);
        break;
      default:
        Serial.print("UNKNOWN PACKAGE ID: ");
        Serial.println(data[0], DEC);
    }

  } else if(type == WS_EVT_DISCONNECT){
    clientOnline = false;
  }
}
