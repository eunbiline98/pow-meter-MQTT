#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <PhiloinDMD.h> // Include lib & font
#include <fonts/SystemFont5x7.h>

const char *SSID = "Padi_solution";
const char *PASS = "s0lut10npadinet";
const char *MQTT_SERVER = "192.168.8.140";

const char *MQTT_POW_CMD = "pow/data/cmnd";
const char *MQTT_POW_STAT = "pow/data/stat";

const char *MQTT_STATUS = "display/status/cmnd";

const char *MQTT_CLIENT_ID = "Display_Philoin-xx01";
const char *MQTT_USERNAME = "homeassistant";
const char *MQTT_PASSWORD = "iecohnee7Ipoh6aizeechopheeth5eeCah6dai8eing2aih2Iefe4nig1EeRaej2";

#define relay 0

#define FONT SystemFont5x7
#define WIDTH 1 // Set width & height
#define HEIGHT 1
PhiloinDMD P10(WIDTH, HEIGHT);

float voltage, current, power, energy, frequency, pf;
String st;
int ResetCounter = 0;
int menuCounter = 0;
int wifiStatus;

unsigned long startMillisPublishPowerMeter;
unsigned long currentMillisPublishPowerMeter;

unsigned long startMillismenu;
unsigned long currentMillismenu;

const char *inputStatus;

IPAddress ip(192, 168, 8, 155);
IPAddress gateway(192, 168, 8, 1);  // Sesuaikan dengan gateway AP
IPAddress subnet(255, 255, 255, 0); // Sesuaikan dengan subnet AP

WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer TelnetServer(8266);

byte progressBar[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};

void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  WiFi.config(ip, gateway, subnet);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
}

void Scrolling_text(int text_height, int scroll_speed, String scroll_text)
{
  static uint32_t pM;
  pM = millis();
  static uint32_t x = 0;
  scroll_text = scroll_text + " ";

  bool scrl_while = 1;
  int dsp_width = P10.width();
  int txt_width = P10.textWidth(scroll_text);

  while (scrl_while == 1)
  {

    P10.loop();

    delay(1);
    if (millis() - pM > scroll_speed)
    {
      P10.setFont(FONT);
      P10.drawText(dsp_width - x, text_height, scroll_text);
      client.loop();

      x++;
      if (x > txt_width + dsp_width)
      {

        x = 0;
        scrl_while = 0;
      }
      pM = millis();
    }
  }
}

void dataPOW()
{
  const size_t capacity = 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(6) + 120;
  DynamicJsonBuffer jsonBuffer(capacity);

  // const char *json = "{\"ZbReceived\":{\"0xB160\":{\"Device\":\"0xB160\",\"Name\":\"Room_Service\",\"Power\":1,\"0006/F000\":61956349,\"Endpoint\":2,\"LinkQuality\":120}}}";

  JsonObject &root = jsonBuffer.parseObject(inputStatus);

  float POWER = root["POWER"];
  float VOLT = root["VOLT"];
  float AMP = root["AMP"];
  float ENERGY = root["ENERGY"];
  float FREQUENCY = root["FREQUENCY"];
  float PF = root["PF"];
  const char *STATUS = root["STATUS"];

  Serial.println();
  Serial.print("volt : ");
  Serial.println(VOLT);
  Serial.print("ampare : ");
  Serial.println(AMP);
  Serial.print("watt : ");
  Serial.println(POWER);
  Serial.print("Freq : ");
  Serial.println(FREQUENCY);
  Serial.print("PF : ");
  Serial.println(PF);
  Serial.print("KwH : ");
  Serial.println(ENERGY);
  // Scrolling_text(0, 50, String(VOLT).c_str());

  currentMillismenu = millis();
  if (currentMillismenu - startMillismenu > 5000)
  {
    startMillismenu = currentMillismenu;
    P10.clear();
    if (menuCounter < 5)
    {
      menuCounter = menuCounter + 1;
    }
    else
    {
      menuCounter = 1;
    }

    switch (menuCounter)
    {
    case 1:
    {
      P10.drawText(5, 0, "Volt");               // P10.drawText(position x , position y, String type text)
      P10.drawText(2, 8, String(VOLT).c_str()); // P10.drawText(position x , position y, String type text)
    }
    break;

    case 2:
    {
      P10.drawText(8, 0, "Amp");               // P10.drawText(position x , position y, String type text)
      P10.drawText(4, 8, String(AMP).c_str()); // P10.drawText(position x , position y, String type text)
    }
    break;

    case 3:
    {
      P10.drawText(2, 0, "Power");               // P10.drawText(position x , position y, String type text)
      P10.drawText(2, 8, String(POWER).c_str()); // P10.drawText(position x , position y, String type text)
    }
    break;

    case 4:
    {
      P10.drawText(5, 0, "Freq");                    // P10.drawText(position x , position y, String type text)
      P10.drawText(2, 8, String(FREQUENCY).c_str()); // P10.drawText(position x , position y, String type text)
    }
    break;

    case 5:
    {
      P10.drawText(6, 0, "Padi"); // P10.drawText(position x , position y, String type text)
      P10.drawText(4, 8, "Tech"); // P10.drawText(position x , position y, String type text)
    }
    break;
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    Serial.println();
    if ((char)message[i] != '"')
      messageTemp += (char)message[i];
  }

  if (String(topic) == MQTT_POW_CMD)
  {
    inputStatus = messageTemp.c_str();
    Serial.print("Data POW : ");
    Serial.print(inputStatus);
    Serial.println();
    dataPOW();
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_STATUS, 1, 1, "Offline"))
    {
      Serial.println("connected");
      // subscribe
      client.subscribe(MQTT_POW_CMD);
      client.publish(MQTT_STATUS, "Online", true);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(2, OUTPUT);

  P10.begin(); // Begin the display & font
  P10.setFont(FONT);
  P10.setBrightness(20);      // Set the brightness
  P10.drawText(4, 0, "HI.."); // P10.drawText(position x , position y, String type text);

  setup_wifi();
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type); });
  TelnetServer.begin(); // Necesary to make Arduino Software autodetect OTA device
  ArduinoOTA.onStart([]()
                     { Serial.println("OTA starting..."); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
  ArduinoOTA.begin();

  client.setCallback(callback);
  client.setServer(MQTT_SERVER, 1883);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address:  ");
  Serial.println(WiFi.macAddress());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected())
  {
    reconnect();
    digitalWrite(2, HIGH);
  }
  P10.loop();
  client.loop();
}