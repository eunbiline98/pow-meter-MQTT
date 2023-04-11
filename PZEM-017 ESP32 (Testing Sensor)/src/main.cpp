#include <Arduino.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <ModbusMaster.h>

#define MAX485_DE 26
#define MAX485_RE 25

#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17);
#else
PZEM004Tv30 pzem(Serial2);
#endif

int counter = 90;
char logBuffer[255];
const long utcOffside = 25200;

// PZEM-004
float voltage, current, power, energy, frequency, pf, totalHarga;
float harga_KWh = 1.445; // harga per KWh

// PZEM-017
float PZEMVoltage = 0;
float PZEMCurrent = 0;
float PZEMPower = 0;
float PZEMEnergy = 0;
float percentage;

unsigned long startMillisPZEM;
unsigned long currentMillisPZEM;
const unsigned long periodPZEM = 1000;

unsigned long startMillisReadData;
unsigned long currentMillisReadData;
const unsigned long periodReadData = 1000;
int ResetEnergy = 0;
int a = 1;
unsigned long startMillis1;

static uint8_t pzemSlaveAddr = 0x01;
static uint16_t NewshuntAddr = 0x0000;

ModbusMaster node;
SoftwareSerial PZEMSerial;

void powerMeter()
{
  // Serial.print("Custom Address:");
  // Serial.println(pzem.readAddress(), HEX);

  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();
  totalHarga = energy * harga_KWh;

  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(pf))
  {
    Serial.println("Error Read Data");
  }
  else
  {
    Serial.print("Vac:");
    Serial.print(voltage);
    Serial.println(" V ");

    Serial.print("A: ");
    Serial.print(current);
    Serial.println(" A ");

    Serial.print("Watt: ");
    Serial.print(power);
    Serial.println(" Wh ");

    Serial.print("===================================================");
    Serial.println("");
  }
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void percentage_batt()
{
  percentage = mapfloat(PZEMVoltage, 11.90, 13.60, 0, 100);

  if (percentage >= 100)
  {
    percentage = 100;
  }
  if (percentage <= 0)
  {
    percentage = 1;
  }
}

void preTransmission()
{
  if (millis() - startMillis1 > 5000)
  {
    digitalWrite(MAX485_RE, 1);
    digitalWrite(MAX485_DE, 1);
    delay(1);
  }
}

void postTransmission()
{
  if (millis() - startMillis1 > 5000)
  {
    delay(3);
    digitalWrite(MAX485_RE, 0);
    digitalWrite(MAX485_DE, 0);
  }
}

void setShunt(uint8_t slaveAddr)
{
  static uint8_t SlaveParameter = 0x06;
  static uint16_t registerAddress = 0x0003;

  uint16_t u16CRC = 0xFFFF;
  u16CRC = crc16_update(u16CRC, slaveAddr);
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));

  preTransmission();

  PZEMSerial.write(slaveAddr);
  PZEMSerial.write(SlaveParameter);
  PZEMSerial.write(highByte(registerAddress));
  PZEMSerial.write(lowByte(registerAddress));
  PZEMSerial.write(highByte(NewshuntAddr));
  PZEMSerial.write(lowByte(NewshuntAddr));
  PZEMSerial.write(lowByte(u16CRC));
  PZEMSerial.write(highByte(u16CRC));
  delay(10);
  postTransmission();
  delay(100);
}

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr)
{

  static uint8_t SlaveParameter = 0x06;
  static uint16_t registerAddress = 0x0002;
  uint16_t u16CRC = 0xFFFF;
  u16CRC = crc16_update(u16CRC, OldslaveAddr);
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
  preTransmission();
  PZEMSerial.write(OldslaveAddr);
  PZEMSerial.write(SlaveParameter);
  PZEMSerial.write(highByte(registerAddress));
  PZEMSerial.write(lowByte(registerAddress));
  PZEMSerial.write(highByte(NewslaveAddr));
  PZEMSerial.write(lowByte(NewslaveAddr));
  PZEMSerial.write(lowByte(u16CRC));
  PZEMSerial.write(highByte(u16CRC));
  delay(10);
  postTransmission();
  delay(100);
}

void setup()
{
  startMillis1 = millis();

  Serial.begin(9600);
  PZEMSerial.begin(9600, SWSERIAL_8N2, 33, 27); // 4 = Rx/R0/ GPIO 4 (D2) & 0 = Tx/DI/ GPIO 0 (D3) on NodeMCU

  startMillisPZEM = millis();
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE, 0);
  digitalWrite(MAX485_DE, 0);

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  node.begin(pzemSlaveAddr, PZEMSerial);
  delay(1000);

  startMillisReadData = millis();
}

void loop()
{
  currentMillisPZEM = millis();
  percentage_batt();
  if (currentMillisPZEM - startMillisPZEM >= periodPZEM)
  {
    uint8_t result;
    result = node.readInputRegisters(0x0000, 6);
    if (result == node.ku8MBSuccess)
    {
      uint32_t tempdouble = 0x00000000;
      PZEMVoltage = node.getResponseBuffer(0x0000) / 100.0;

      PZEMCurrent = node.getResponseBuffer(0x0001) / 100.0;

      tempdouble = (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002);
      PZEMPower = tempdouble / 10.0;

      tempdouble = (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004);
      PZEMEnergy = tempdouble;

      if (pzemSlaveAddr == 2) /* just for checking purpose to see whether can read modbus*/
      {
      }
    }
    else
    {
    }
    startMillisPZEM = currentMillisPZEM;
  }

  currentMillisReadData = millis();
  if (currentMillisReadData - startMillisReadData >= periodReadData)
  {
    powerMeter();
    percentage_batt();

    Serial.print("Vdc : ");
    Serial.print(PZEMVoltage);
    Serial.println(" V ");

    Serial.print("Idc : ");
    Serial.print(PZEMCurrent);
    Serial.println(" A ");

    Serial.print("Power : ");
    Serial.print(PZEMPower);
    Serial.println(" W ");

    Serial.print("Energy : ");
    Serial.print(PZEMEnergy);
    Serial.println(" Wh ");

    Serial.print("Percentage : ");
    Serial.print(percentage);
    Serial.println(" % ");
    Serial.println("=========================================");

    startMillisReadData = millis();
  }
}
