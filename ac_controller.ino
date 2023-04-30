// Must define 'NETWORK_SSID' and 'NETWORK_PW'
#include "ac_controller.h"

#include <ArduinoJson.h>
#include <ir_Daikin.h>
#include <WebServer.h>
#include <WiFi.h>

const uint16_t led_out = D0;
IRDaikinESP ac(led_out);

WebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(NETWORK_SSID, NETWORK_PW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ac.begin();

  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send(
      200,
      "text/plain",
      "AC controller running: \n"
      "\"powerStatus\": [\"on\", \"off\"]\n"
      "\"mode\": [\"auto\", \"dry\", \"heat\", \"cool\", \"fan\"]\n"
      "\"temp\": [15, 30], omitted if mode is fan\n"
      "\"fanMode\": [\"1\", \"2\", \"3\", \"4\", \"5\", \"auto\", \"quiet\"]\n"
      "\"timerOn\": run after number of minutes\n"
      "\"timerOff\": stop after number of minutes\n"
      "\"swing\": true, omit for false\n"
      "\"comfort\": true, omit for false"
    );
  });

  server.on("/", HTTP_PUT, []() {
    DynamicJsonDocument arguments(1024);
    DeserializationError error = deserializeJson(arguments, server.arg("plain"));
    if (error) {
      server.send(404, "text/plain", error.c_str());
      return;
    }

    if (arguments.containsKey("powerStatus")) {
      if (arguments["powerStatus"] == "on") {
        ac.on();
      } else {
        ac.off();
      }
    }

    if (arguments.containsKey("mode")) {
      if (arguments["mode"] == "auto") {
        ac.setMode(kDaikinAuto);
      } else if (arguments["mode"] == "dry") {
        ac.setMode(kDaikinDry);
      } else if (arguments["mode"] == "heat") {
        ac.setMode(kDaikinHeat);
      } else if (arguments["mode"] == "cool") {
        ac.setMode(kDaikinCool);
      } else if (arguments["mode"] == "fan") {
        ac.setMode(kDaikinFan);
      }
      ac.setTemp((arguments.containsKey("temp")) ? arguments["temp"] : 25);
    }

    if (arguments.containsKey("fanMode")) {
      if (arguments["fanMode"] == "auto") {
        ac.setFan(kDaikinFanAuto);
      } else if (arguments["fanMode"] == "quiet") {
        ac.setFan(kDaikinFanQuiet);
      } else {
        ac.setFan((uint8_t) arguments["fanMode"]);
      }
    }

    ac.setCurrentTime(0);

    arguments.containsKey("timerOn") ? ac.enableOnTimer((arguments["timerOn"])) : ac.disableOnTimer();
    arguments.containsKey("timerOff") ? ac.enableOffTimer(arguments["timerOff"]) : ac.disableOffTimer();

    ac.setSwingVertical(arguments.containsKey("swing"));
    ac.setComfort(arguments.containsKey("comfort"));
    ac.setWeeklyTimerEnable(false);

    ac.send();

    Serial.println(ac.toString());

    server.send(200, "text/plan", ac.toString());
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
