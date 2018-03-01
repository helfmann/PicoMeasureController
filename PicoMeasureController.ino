#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <stdio.h>
#include <stdlib.h>
#include <SPI.h>

#define DAC1 4        //This will be defined as TL
#define DAC2 5        //This will be defined as TR
#define DAC3 6        //This will be defined as BL
#define DAC4 7        //This will be defined as BR
#define W_CLK 8       // Pin 8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9       // Pin 9 - connect to freq update pin (FQ)
#define DATA 10       // Pin 10 - connect to serial data load pin (DATA)
#define RESET 11      // Pin 11 - connect to reset pin (RST).


#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

Adafruit_MCP4725 dac;
String input;
String DAC_SELECT;
String data;
String CND_TYPE; // Condition type defining Voltage or Pressure, May be removed

String incomingByte; //defines incoming sring
String MSG_type; //Start of input message, first character
String CND_type_0, CND_type_1; //Condition Message type, str or end
long unsigned int freq = 1000;
long unsigned int FreqOld = freq;
String Start_Freq;
String End_Freq;
long unsigned int Start;
long unsigned int End;
int Sweep;

void tfr_byte(byte data)
{
  for (int i = 0; i < 8; i++, data >>= 1) {
    digitalWrite(DATA, data & 0x01);
    pulseHigh(W_CLK);   //after each bit sent, CLK is pulsed high
  }
}

void sendFrequency(double frequency) {
  int32_t freq = frequency * 4294967295 / 125000000; // note 125 MHz clock on 9850
  for (int b = 0; b < 4; b++, freq >>= 8) {
    tfr_byte(freq & 0xFF);
  }
  tfr_byte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}

void setup() {
  // put your setup code here, to run once:
  pinMode(DAC1, OUTPUT);
  pinMode(DAC2, OUTPUT);
  pinMode(DAC3, OUTPUT);
  pinMode(DAC4, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(RESET, OUTPUT);

  pulseHigh(RESET);
  pulseHigh(W_CLK);
  pulseHigh(FQ_UD);

  Serial.begin(9600);
  Serial.println("Hello there!");
  dac.begin(0x62);
  dac.setVoltage(0, true);
  dac.begin(0x63);
}

void loop() {
  if (Serial.available() > 0) {

    
    incomingByte = Serial.readString();
    input = incomingByte;
    MSG_type = incomingByte.substring(0, 2);
    String    format1 = input.substring(6, 8);
    String    format2 = input.substring(12, 14);
    String   format3 = input.substring(18, 20);
    if (MSG_type == "TL" && format1 == "TR" && format2 == "BL" && format3 == "BR") {
      String num1 = input.substring(2,6);
      float  p1 = num1.toFloat();
      String num2 = input.substring(8,12);
      float  p2 = num2.toFloat();
      String num3 = input.substring(14,18);
      float  p3 = num3.toFloat();
      String num4 = input.substring(20,24);
      float  p4 = num4.toFloat();
      p1 = p1 / 100;
      p2 = p2 / 100;
      p3 = p3 / 100;
      p4 = p4 / 100;
      if (p1 > 55 || p2 > 55 || p3 > 55 || p4 > 55 || p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
        Serial.println("Either that pressure is too low or you want to blow up the machine, please dont...");


      }
      else {
        float  v1 =  0.01776 + 0.14722 * p1;
        v1 = - 1.33332 + v1 * 413.973996;
        float  v2 =  0.01776 + 0.14722 * p2;
        v2 = - 1.33332 + v2 * 413.973996;
        float  v3 =  0.01776 + 0.14722 * p3;
        v3 = - 1.33332 + v3 * 413.973996;
        float  v4 =  0.01776 + 0.14722 * p4;
        v4 = - 1.33332 + v4 * 413.973996;
        digitalWrite(DAC1, HIGH);
        dac.setVoltage(v1, false);
        delay(1);
        digitalWrite(DAC1, LOW);

        digitalWrite(DAC2, HIGH);
        dac.setVoltage(v2, false);
        delay(1);
        digitalWrite(DAC2, LOW);

        digitalWrite(DAC3, HIGH);
        dac.setVoltage(v3, false);
        delay(1);
        digitalWrite(DAC3, LOW);

        digitalWrite(DAC4, HIGH);
        dac.setVoltage(v4, false);
        delay(1);
        digitalWrite(DAC4, LOW);

      }
    }

    if (MSG_type == "sS" || MSG_type == "fS") {
      CND_type_1 = incomingByte.substring(5, 6);
      Start_Freq = incomingByte.substring(2, 5);
      End_Freq = incomingByte.substring(6, 9);
    }

    if (MSG_type == "sS" &&  CND_type_1 == "E" ) { // figure out if the setting is swing or fixed
      Start = Start_Freq.toInt();
      End = End_Freq.toInt();
      Start *= 1000;
      End *= 1000;
      freq = Start;
      Sweep = 1;
    }
    else if (MSG_type == "fS") {
      Start = Start_Freq.toInt();
      Start *= 1000;
      freq = Start;
      sendFrequency(freq);
      Sweep = 0;
    }
    if (incomingByte == "pstop"){
      digitalWrite(DAC1, HIGH);
      digitalWrite(DAC2, HIGH);
      digitalWrite(DAC3, HIGH);
      digitalWrite(DAC4, HIGH);
      dac.setVoltage(0, false);
      delay(1);
      digitalWrite(DAC1, LOW);
      digitalWrite(DAC2, LOW);
      digitalWrite(DAC3, LOW);
      digitalWrite(DAC4, LOW);
    }
    if (incomingByte == "fstop"){
      sendFrequency(0);
      Sweep = 0;
    }

  }

  if (Sweep = 1) {
    for (freq = Start; freq <= End; freq += 100) { //this is for sweep frequency
      sendFrequency(freq);  // freq
      delay(10);
    }
    freq = Start;
  }


}

