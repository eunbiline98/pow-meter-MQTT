#include <Arduino.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>

#define relay 0

float voltage, current, power, energy, frequency, pf;
String st;
String st_mesin;
int ResetCounter = 0;
int menuCounter = 0;
int wifiStatus;

unsigned long startMillisPublishPowerMeter;
unsigned long currentMillisPublishPowerMeter;

unsigned long startMillismenu;
unsigned long currentMillismenu;

PZEM004Tv30 pzem;
SoftwareSerial pzemSerial(D5, D6); // RX, TX
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte progressBar[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};

void powerMeter()
{
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

  // if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf))
  // {
  //   st = "Err";
  //   lcd.setCursor(1, 0);
  //   lcd.print("Device Status");
  //   lcd.setCursor(5, 1);
  //   lcd.print("Error");
  //   // client.publish(MQTT_POW_MSG_CMD, "Err");
  // }
  // else
  // {
  //   st = "OK";
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
  { // Design of page 4
    if (current == 0)
    {
      lcd.setCursor(1, 0);
      lcd.print("Machine Status");
      lcd.setCursor(3, 1);
      lcd.print("Engine Off");
    }
    else
    {
      lcd.setCursor(1, 0);
      lcd.print("Machine Status");
      lcd.setCursor(3, 1);
      lcd.print("Engine On");
    }
  }
  break;

  case 5:
  { // Design of page 4
    lcd.setCursor(2, 0);
    lcd.print("Power Meter");
    lcd.setCursor(4, 1);
    lcd.print("Ver 1.0");
  }
  break;
  }
  // }
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
  delay(2000);
  lcd.clear();
}

void loop()
{
  currentMillisPublishPowerMeter = millis();
  if (currentMillisPublishPowerMeter - startMillisPublishPowerMeter >= 2000)
  {
    powerMeter();
    startMillisPublishPowerMeter = millis();
  }
  digitalWrite(2, LOW);
}