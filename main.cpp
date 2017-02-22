/*
 Raspberry Pi 3 control code for a humidifier.

    Copyright (C) 2017 Kennesaw State University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <QCoreApplication>
#include <iostream>
#include <iomanip>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mcp3004.h>

#include "timer.h"

using namespace std;

#ifndef TOLERANCE
#define TOLERANCE 2.0 // 2 degrees F
#endif

#define RELAYPIN 14 // the 5v relay module is on this GPIO pin, it controls the fan
#define DHTPIN 23 // the pin for the temp/humidity sensor
#define MAXTIMINGS 85
#define MAXHISTORICAL 14
#define BUZZERPIN 18
#define WATERLEDPIN 16

#define RT_DIGIT_SET_VALUE 4
#define LT_DIGIT_SET_VALUE 17

#define RT_DIGIT_TEMP_VALUE 20
#define LT_DIGIT_TEMP_VALUE 21

#define RT_DIGIT_HUM_VALUE 27
#define LT_DIGIT_HUM_VALUE 22

#define DIGIT8 13
#define DIGIT4 26
#define DIGIT2 19
#define DIGIT1 6

#define LEFT 10
#define RIGHT 1

#define TEMPERATURE 1
#define HUMIDSENSOR 2
#define HUMIDSETTER 3

bool _fanOn = false;

void SetAllDisplayLow(){
    digitalWrite(RT_DIGIT_HUM_VALUE, LOW);
    digitalWrite(LT_DIGIT_HUM_VALUE, LOW);
    digitalWrite(RT_DIGIT_SET_VALUE, LOW);
    digitalWrite(LT_DIGIT_SET_VALUE, LOW);
    digitalWrite(RT_DIGIT_TEMP_VALUE, LOW);
    digitalWrite(LT_DIGIT_TEMP_VALUE, LOW);
}

void SetAllDisplayHigh(){
    digitalWrite(RT_DIGIT_HUM_VALUE, HIGH);
    digitalWrite(LT_DIGIT_HUM_VALUE, HIGH);
    digitalWrite(RT_DIGIT_SET_VALUE, HIGH);
    digitalWrite(LT_DIGIT_SET_VALUE, HIGH);
    digitalWrite(RT_DIGIT_TEMP_VALUE, HIGH);
    digitalWrite(LT_DIGIT_TEMP_VALUE, HIGH);
}

void ResetAllDigits() {
    SetAllDisplayHigh();
    digitalWrite(DIGIT1, HIGH);
    digitalWrite(DIGIT2, HIGH);
    digitalWrite(DIGIT4, HIGH);
    digitalWrite(DIGIT8, HIGH);
    SetAllDisplayLow();
}

void CheckAndSet(int ValueToset, int CurrentBit, int PinDigit){
    if((int)(ValueToset|CurrentBit) == ValueToset){
        digitalWrite(PinDigit, HIGH);
    } else {
        digitalWrite(PinDigit, LOW);
    }
}
void SetDigit(int ValueToSet) {
    CheckAndSet(ValueToSet, 8, DIGIT8);
    CheckAndSet(ValueToSet, 4, DIGIT4);
    CheckAndSet(ValueToSet, 2, DIGIT2);
    CheckAndSet(ValueToSet, 1, DIGIT1);
}
void SetValue(int DisplaySet, int Value, int LeftOrRight){
    SetAllDisplayLow();
    if (DisplaySet == TEMPERATURE){
        if(LeftOrRight == LEFT){
            digitalWrite(LT_DIGIT_TEMP_VALUE, HIGH);
        } else {
            digitalWrite(RT_DIGIT_TEMP_VALUE, HIGH);
        }
    } else if (DisplaySet == HUMIDSENSOR) {
        if (LeftOrRight == LEFT) {
            digitalWrite(LT_DIGIT_HUM_VALUE, HIGH);
        } else {
            digitalWrite(RT_DIGIT_HUM_VALUE, HIGH);
        }
    } else if (DisplaySet == HUMIDSETTER) {
        if (LeftOrRight == LEFT) {
            digitalWrite(LT_DIGIT_SET_VALUE, HIGH);
        } else {
            digitalWrite(RT_DIGIT_SET_VALUE, HIGH);
        }
    }
    SetDigit(Value);
    SetAllDisplayLow();
}

void SetLeftValue(int DisplaySet, int Value){
    SetValue(DisplaySet, Value, LEFT);
}
void SetRightValue(int DisplaySet, int Value){
    SetValue(DisplaySet, Value, RIGHT);
}
void SetDisplayValue(int DisplaySet, int Value){
    int LeftValue = Value / 10;
    int RightValue = Value % 10;
    SetLeftValue(DisplaySet, LeftValue);
    delay(1);
    SetRightValue(DisplaySet, RightValue);
    delay(1);
}
void SetUpPins() {
    pinMode(DIGIT8, OUTPUT);
    pinMode(DIGIT4, OUTPUT);
    pinMode(DIGIT2, OUTPUT);
    pinMode(DIGIT1, OUTPUT);
    pinMode(LT_DIGIT_HUM_VALUE, OUTPUT);
    pinMode(RT_DIGIT_HUM_VALUE, OUTPUT);
    pinMode(LT_DIGIT_SET_VALUE, OUTPUT);
    pinMode(RT_DIGIT_SET_VALUE, OUTPUT);
    pinMode(LT_DIGIT_TEMP_VALUE, OUTPUT);
    pinMode(RT_DIGIT_TEMP_VALUE, OUTPUT);
    ResetAllDigits();
    SetAllDisplayLow();
}

void Beep(int reps, int sleep)
{
    for(int i = 0; i < reps; i++)
    {
        digitalWrite(BUZZERPIN, HIGH);
        delay(sleep);
        digitalWrite(BUZZERPIN, LOW);
        delay(sleep);
    }
}

void TurnOnFan() {
    digitalWrite(RELAYPIN, LOW); // disable for now
    //cout << "Humidifier fan on" << endl;
    _fanOn = true;
}

void TurnOffFan() {
    digitalWrite(RELAYPIN, HIGH);
    //cout << "Humidifier fan off" << endl;
    _fanOn = false;
}

double GetAverage(double readings[MAXHISTORICAL]) {
    double avg = 0.0;

    for (int i = 0; i < MAXHISTORICAL; i++) {
        avg = avg + readings[i];
    }

    return avg / MAXHISTORICAL;
}

void ShiftReadings(double readings[MAXHISTORICAL]) {
    for (int i = 1; i < MAXHISTORICAL; i++) {
        readings[i - 1] = readings[i];
    }
}

int dht11_dat[5] = { 0, 0, 0, 0, 0 };

bool GetCurrentReadings(double &temp, double &humidity) {

    uint8_t laststate	= HIGH;
    uint8_t counter		= 0;
    uint8_t j		= 0, i;
    float	f;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    // pull pin down for 18 milliseconds
        pinMode(DHTPIN, OUTPUT);
        digitalWrite(DHTPIN, LOW);
        delay(18);
        // then pull it up for 40 microseconds
        digitalWrite(DHTPIN, HIGH);
        delayMicroseconds(40);
        // prepare to read the pin
        pinMode(DHTPIN, INPUT);

    //reads %RH and Temp. (F & C)
    for ( i=0; i< MAXTIMINGS; i++) {
            counter = 0;
            while (digitalRead(DHTPIN) == laststate) {
                counter++;
                delayMicroseconds(1);
                if (counter == 255) {
                    break;
                }
            }
            laststate = digitalRead(DHTPIN);

            if (counter == 255) break;

            // ignore first 3 transitions
            if ((i >= 4) && (i%2 == 0)) {
                // shove each bit into the storage bytes
                dht11_dat[j/8] <<= 1;
                if (counter > 16)
                    dht11_dat[j/8] |= 1;
                j++;
            }
        }

    if ((j >= 40) &&
                (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)) ) {
            f = dht11_dat[2] * 9. / 5. + 32;

        /*
            printf( "  Sensor reading (DHT11): Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
            dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
        */

        temp = f;
        humidity = dht11_dat[0];
        return true;
    }
    else
    {
        //printf( "  Sensor reading (DHT11): Bad data read from DHT11, skipping\n" );
        return false;
    }
}

double GetWaterLevel() {

    int currentReading = analogRead(101);

    int newWaterLevel = (currentReading*100)/1023 - 1;
    //printf("  Sensor reading (Water): %d\n", newWaterLevel);

    return newWaterLevel + 0.0;
}

int GetHumiditySetLevel() {

    int currentReading = analogRead(100);
    int newHumidity = (currentReading*100)/1023 - 1;
    //printf("  Dial reading: %d\n", newHumidity);

    return newHumidity;
}

bool IsOutOfWater(double historicWaterLevels[MAXHISTORICAL]) {
    double avg = GetAverage(historicWaterLevels);

    if (avg < 10) { // arbitrary, may need adjusting
        return true;
    }
    else {
        return false;
    }
}

void SetWaterLight(bool isOn) {
    if (isOn) {
        digitalWrite(WATERLEDPIN, HIGH);
    }
    else {
        digitalWrite(WATERLEDPIN, LOW);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cout << "Initializing..." << endl;

    if (wiringPiSetupGpio() == -1) {
        cout << "ERROR: WiringPi Setup Gpio failed!" << endl;
        exit(1);
    }

    cout << "Wiring Pi setup completed" << endl;

    pinMode(RELAYPIN, OUTPUT);
    digitalWrite(RELAYPIN, HIGH); // fan starts in OFF position!

    pinMode(BUZZERPIN, OUTPUT);
    pinMode(WATERLEDPIN, OUTPUT);

    digitalWrite(WATERLEDPIN, LOW);

    SetUpPins();
    ResetAllDigits();

    cout << "Relay PIN set" << endl;

    mcp3004Setup(100, 0);

    double waterLevel = GetWaterLevel();

    double pastTempReadings [MAXHISTORICAL];
    double pastHumidityReadings [MAXHISTORICAL];
    double pastWaterReadings [MAXHISTORICAL];

    double curTemp = 0.0;
    double curHumidity = 0.0;

    cout << "Getting baseline temperature and humidity readings..." << endl;

    while (true)
    {
        if (GetCurrentReadings(curTemp, curHumidity) == true)
        {
            for (int i = 0; i < MAXHISTORICAL; i++) {
                pastTempReadings[i] = curTemp;
                pastHumidityReadings[i] = curHumidity;
            }
            break;
        }
        delay(2000);
    }
    cout << "Baseline temperature and humidity set" << endl;

    for (int i = 0; i < MAXHISTORICAL; i++) {
        pastWaterReadings[i] = waterLevel;
    }

    double setHumidity = 0.0;

    cout << "Starting timer..." << endl;
    Timer timer = Timer(2000);
    timer.Start();
    cout << "Timer set" << endl;

    cout << "Running control code..." << endl;

    bool outOfWater = false;
    bool lastOutOfWater = false;

    cout << endl;

    for (int k = 0; k < 50; k++)
    {
        cout << "H (user): " << setprecision(2) << setHumidity << "   H (sensor): " << setprecision(2) << curHumidity << "   Wtr: " << setprecision(2) << pastWaterReadings[0] << "   Fan: ";

        if (_fanOn == true) cout << "ON                 ";
        else if (_fanOn == false && outOfWater == true) cout << "OFF (no water)             ";
        else if (_fanOn == false) cout << "OFF                 ";
        cout << "\r";
    }

    auto avgHumidity = GetAverage(pastHumidityReadings);
    auto avgTemp = GetAverage(pastTempReadings);
    auto avgWater = GetAverage(pastWaterReadings);
    setHumidity = GetHumiditySetLevel() + 0.0;
    double lastSetHumidity = setHumidity;

    SetDisplayValue(HUMIDSETTER, setHumidity);

    while (true) {

        // update potentiometer outside of timer loop. Loop is only for measurements!
        setHumidity = GetHumiditySetLevel() + 0.0;

        SetDisplayValue(TEMPERATURE, avgTemp);
        SetDisplayValue(HUMIDSENSOR, avgHumidity);
        // Only change the set humidity if needed
        if (setHumidity != lastSetHumidity) {
            SetDisplayValue(HUMIDSETTER, setHumidity);
            lastSetHumidity = setHumidity;
        }

        delay(5);

        if (timer.GetIsElapsed() == true)
        {
            ShiftReadings(pastWaterReadings);
            pastWaterReadings[MAXHISTORICAL - 1] = GetWaterLevel();
            outOfWater = IsOutOfWater(pastWaterReadings);

            bool valid = GetCurrentReadings(curTemp, curHumidity);

            delay(20);

            if (valid) {
                ShiftReadings(pastHumidityReadings);
                ShiftReadings(pastTempReadings);

                pastHumidityReadings[MAXHISTORICAL - 1] = curHumidity;
                pastTempReadings[MAXHISTORICAL - 1] = curTemp;
            }

            avgHumidity = GetAverage(pastHumidityReadings);
            avgTemp = GetAverage(pastTempReadings);
            avgWater = GetAverage(pastWaterReadings);

            for (int k = 0; k < 50; k++)
            {
                cout << "H (user): " << setprecision(2) << setHumidity << "   H (sensor): " << setprecision(2) << avgHumidity << "   Wtr: " << setprecision(2) << avgWater << "   Fan: ";

                if (_fanOn == true) cout << "ON        ";
                else if (_fanOn == false && outOfWater == true) cout << "OFF (no water)         ";
                else if (_fanOn == false) cout << "OFF         ";
                cout << "\r";
            }

            if (!outOfWater)
            {
                // If the running average over past MAXHISTORICAL readings is less than the SET value minus the allowable TOLERANCE, then turn on the fan
                if (avgHumidity < setHumidity - TOLERANCE)
                {
                    if (_fanOn == false)
                    {
                        TurnOnFan();
                    }
                }
                else
                {
                    if (_fanOn == true)
                    {
                        TurnOffFan();
                    }
                }

                if (outOfWater == false && lastOutOfWater == true)
                {
                    SetWaterLight(false);
                }
            }
            else
            {
                // First cycle after water basin is empty, so auto-shutoff the fan and beep
                if (lastOutOfWater == false && outOfWater == true)
                {
                    if (_fanOn == true)
                    {
                        TurnOffFan();
                    }

                    // so we just ran out of water
                    SetWaterLight(true);
                    Beep(10, 150);
                    cout << endl << "Water basin is empty; refill!" << endl;
                }
            }

            timer = Timer(2000);
            timer.Start();

            lastOutOfWater = outOfWater;
        }
    }

    return a.exec();
}


