#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

const char *SSID = "Padi_solution";
const char *PASS = "s0lut10npadinet";
const char *MQTT_SERVER = "192.168.8.140";

const char *MQTT_POW_CMD = "pow/data/cmnd";
const char *MQTT_POW_STAT = "pow/data/stat";

const char *MQTT_POW_MSG_CMD = "pow/msg/cmnd";
const char *MQTT_POW_MSG_STAT = "pow/msg/stat";

const char *MQTT_POW_RST_CMD = "pow/rst/cmnd";
const char *MQTT_POW_RST_STAT = "pow/rst/stat";

const char *MQTT_POW_RELAY_CMD = "pow/relay/cmnd";
const char *MQTT_POW_RELAY_STAT = "pow/relay/stat";

const char *MQTT_STATUS = "pow/status/cmnd";

const char *MQTT_CLIENT_ID = "POW_Philoin-xx01";
const char *MQTT_USERNAME = "homeassistant";
const char *MQTT_PASSWORD = "iecohnee7Ipoh6aizeechopheeth5eeCah6dai8eing2aih2Iefe4nig1EeRaej2";

#define relay 0

float voltage, current, power, energy, frequency, pf;
String st;
int ResetCounter = 0;
int menuCounter = 0;
int wifiStatus;

unsigned long startMillisPublishPowerMeter;
unsigned long currentMillisPublishPowerMeter;

unsigned long startMillismenu;
unsigned long currentMillismenu;

IPAddress ip(192, 168, 8, 188);
IPAddress gateway(192, 168, 8, 188); // Sesuaikan dengan gateway AP
IPAddress subnet(255, 255, 255, 0);  // Sesuaikan dengan subnet AP

WiFiClient espClient;
PubSubClient client(espClient);
PZEM004Tv30 pzem;
SoftwareSerial pzemSerial(D5, D6); // RX, TX
LiquidCrystal_I2C lcd(0x27, 16, 2);
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
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");
  }
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.print("Connection Failed!");
    delay(5000);
    ESP.restart();
  }
}

void powerMeter()
{
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

  StaticJsonBuffer<300> JSONbuffer;
  JsonObject &JSONencoder = JSONbuffer.createObject();

  JSONencoder["POWER"] = power;
  JSONencoder["VOLT"] = voltage;
  JSONencoder["AMP"] = current;
  JSONencoder["ENERGY"] = energy;
  JSONencoder["FREQUENCY"] = frequency;
  JSONencoder["PF"] = pf;
  JSONencoder["STATUS"] = st;

  // JsonArray &values = JSONencoder.createNestedArray("values");
  // values.add(20);
  // values.add(21);
  // values.add(23);

  char JSONmessageBuffer[200];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);

  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf))
  {
    st = "Err";
    lcd.setCursor(1, 0);
    lcd.print("Device Status");
    lcd.setCursor(5, 1);
    lcd.print("Error");
    client.publish(MQTT_POW_CMD, JSONmessageBuffer);
    // client.publish(MQTT_POW_MSG_CMD, "Err");
  }
  else
  {
    st = "OK";
    currentMillismenu = millis();
    if (currentMillismenu - startMillismenu > 3000)
    {
      startMillismenu = currentMillismenu;
      lcd.clear();
      if (menuCounter < 5)
      {
        menuCounter = menuCounter + 1;
      }
      else
      {
        menuCounter = 1;
      }
    }

    switch (menuCounter)
    {
    case 1:
    { // Design of home page 1
      lcd.setCursor(1, 0);
      lcd.print("V:");
      lcd.print(voltage, 1);

      lcd.setCursor(9, 0);
      lcd.print("A:");
      lcd.print(current, 2);

      lcd.setCursor(1, 1);
      lcd.print("W:");
      lcd.print(power, 1);

      lcd.setCursor(9, 1);
      lcd.print("P:");
      lcd.print(pf, 1);
    }
    break;

    case 2:
    { // Design of page 2
      lcd.setCursor(0, 0);
      lcd.print("Pwr Consumption");
      lcd.setCursor(5, 1);
      lcd.print(energy, 3);
    }
    break;

    case 3:
    { // Design of page 2
      lcd.setCursor(3, 0);
      lcd.print("Frequecny");
      lcd.setCursor(6, 1);
      lcd.print(frequency, 1);
    }
    break;

    case 4:
    { // Design of page 3
      wifiStatus = WiFi.status();
      if (wifiStatus == 3)
      {
        lcd.setCursor(1, 0);
        lcd.print("Device Status");
        lcd.setCursor(3, 1);
        lcd.print("Connected");
      }
      else
      {
        lcd.setCursor(1, 0);
        lcd.print("Device Status");
        lcd.setCursor(1, 1);
        lcd.print("Disconnected");
      }
    }
    break;

    case 5:
    { // Design of page 4
      lcd.setCursor(3, 0);
      lcd.print("Product By");
      lcd.setCursor(1, 1);
      lcd.print("Padi Technolgy");
    }
    break;
    }

    // client.publish(MQTT_POW_CMD, JSONmessageBuffer);
    if (client.publish(MQTT_POW_CMD, JSONmessageBuffer) == true)
    {
      Serial.println("Success sending message");
    }
    else
    {
      Serial.println("Error sending message");
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

  if (String(topic) == MQTT_POW_RELAY_CMD)
  {
    if (messageTemp == "1")
    {
      digitalWrite(relay, HIGH);
    }

    else if (messageTemp == "0")
    {
      digitalWrite(relay, LOW);
    }
  }

  if (String(topic) == MQTT_POW_RST_CMD)
  {
    if (messageTemp == "1")
    {
      pzem.resetEnergy();
      for (int i = 0; i <= 3; i++)
      {
        lcd.setCursor(0, 0);
        lcd.print("    KwH Meter    ");
        lcd.setCursor(0, 1);
        lcd.print("      Reset      ");
        lcd.noBacklight();
        delay(500);
        lcd.backlight();
        delay(500);
      }
      client.publish(MQTT_POW_RST_CMD, "0");
    }
    else if (messageTemp == "0")
    {
      lcd.backlight();
    }
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
      client.subscribe(MQTT_POW_RELAY_CMD);
      client.subscribe(MQTT_POW_RST_CMD);
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
  pzem = PZEM004Tv30(pzemSerial);
  lcd.init();
  lcd.backlight();
  pinMode(2, OUTPUT);
  pinMode(relay, OUTPUT);

  lcd.setCursor(2, 0);
  lcd.print("Power Meter");
  lcd.setCursor(4, 1);
  lcd.print("Ver 1.0");
  delay(5000);

  lcd.createChar(0, progressBar);
  lcd.setCursor(0, 1);
  lcd.print("                   ");
  for (int i = 0; i <= 16; i++)
  {
    lcd.setCursor(0, 0);
    lcd.print("initialize...!!!       ");
    lcd.setCursor(i, 1);
    lcd.write(byte(0));
    delay(120);
  }
  lcd.clear();

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
  lcd.setCursor(0, 0);
  lcd.print("Ip Address         ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected())
  {
    reconnect();
    digitalWrite(2, HIGH);
  }
  client.loop();
  currentMillisPublishPowerMeter = millis();
  if (currentMillisPublishPowerMeter - startMillisPublishPowerMeter >= 2000)
  {
    powerMeter();
    startMillisPublishPowerMeter = millis();
  }
  digitalWrite(2, LOW);
}