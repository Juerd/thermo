#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFiSettings.h>
#include <MQTT.h>

#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

const int buttonpin  = 0;
const int onewirepin = 17;

OneWire ds(onewirepin);
DallasTemperature sensors(&ds);

WiFiClient wificlient;
MQTTClient mqtt;

int    num_sensors;
String topic;
bool   publish_all;

void setup() {
    Serial.begin(115200);
    sensors.begin();
    SPIFFS.begin(true);
    pinMode(buttonpin, INPUT);

    String server = WiFiSettings.string("mqtt_server", 64, "test.mosquitto.org");
    int port      = WiFiSettings.integer("mqtt_port", 0, 65535, 1883);
    topic         = WiFiSettings.string("mqtt_topic", "thermo");
    num_sensors   = WiFiSettings.integer("num_sensors", 1);
    publish_all   = WiFiSettings.checkbox("publish_all", true);

    for (int i = 0; i < 1000; i++) {
        if (!digitalRead(onewirepin)) WiFiSettings.portal();
        delay(1);
    }
    WiFiSettings.connect();

    mqtt.begin(server.c_str(), port, wificlient);
}


void loop() {
    while (!mqtt.connected()) {
        if (!mqtt.connect("")) delay(500);
    }
    sensors.requestTemperatures();
    String all = "";
    for (int i = 0; i < num_sensors; i++) {
        float C = sensors.getTempCByIndex(i);
        Serial.printf("%d: %.2f\n", i, C);
        mqtt.publish(topic + "/" + i, Sprintf("%.2f", C));
        if (all.length()) all += ";";
        all += Sprintf("%.2f", C);
        delay(100);
    }
    if (publish_all) mqtt.publish(topic + "/all", all);

    delay(1000 - num_sensors * 100);
}
