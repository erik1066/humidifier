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
#define WATERLEDPIN 24

#define RT_DIGIT_SET_VALUE 21
#define LT_DIGIT_SET_VALUE 20

#define RT_DIGIT_TEMP_VALUE 27
#define LT_DIGIT_TEMP_VALUE 22

#define RT_DIGIT_HUM_VALUE 12
#define LT_DIGIT_HUM_VALUE 16

bool _fanOn = false;

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

void DisplayDigit(int digit) {

    switch (digit) {
        case -1:
        case 0:
            digitalWrite(6, LOW); // 1
            digitalWrite(13, LOW); // 8
            digitalWrite(19, LOW); // 2
            digitalWrite(26, LOW); // 4
            break;
    case 1:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, LOW); // 2
        digitalWrite(26, LOW); // 4
        break;
    case 2:
        digitalWrite(6, LOW); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, HIGH); // 2
        digitalWrite(26, LOW); // 4
        break;
    case 3:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, HIGH); // 2
        digitalWrite(26, LOW); // 4
        break;
    case 4:
        digitalWrite(6, LOW); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, LOW); // 2
        digitalWrite(26, HIGH); // 4
        break;
    case 5:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, LOW); // 2
        digitalWrite(26, HIGH); // 4
        break;
    case 6:
        digitalWrite(6, LOW); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, HIGH); // 2
        digitalWrite(26, HIGH); // 4
        break;
    case 7:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, LOW); // 8
        digitalWrite(19, HIGH); // 2
        digitalWrite(26, HIGH); // 4
        break;
    case 8:
        digitalWrite(6, LOW); // 1
        digitalWrite(13, HIGH); // 8
        digitalWrite(19, LOW); // 2
        digitalWrite(26, LOW); // 4
        break;
    case 9:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, HIGH); // 8
        digitalWrite(19, LOW); // 2
        digitalWrite(26, LOW); // 4
        break;
    default:
        digitalWrite(6, HIGH); // 1
        digitalWrite(13, HIGH); // 8
        digitalWrite(19, HIGH); // 2
        digitalWrite(26, HIGH); // 4
    }
}

void ResetDigits() {

    double delayTime1 = 0.1;
    double delayTime2 = 0.1;

    digitalWrite(RT_DIGIT_SET_VALUE, LOW);
    digitalWrite(LT_DIGIT_SET_VALUE, LOW);
    digitalWrite(RT_DIGIT_TEMP_VALUE, LOW);
    digitalWrite(LT_DIGIT_TEMP_VALUE, LOW);
    digitalWrite(RT_DIGIT_HUM_VALUE, LOW);
    digitalWrite(LT_DIGIT_HUM_VALUE, LOW);

    delay(delayTime1);

    digitalWrite(RT_DIGIT_SET_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(LT_DIGIT_SET_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(RT_DIGIT_TEMP_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(LT_DIGIT_TEMP_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(RT_DIGIT_HUM_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(LT_DIGIT_HUM_VALUE, HIGH);
    DisplayDigit(-5);
    delay(delayTime2);

    digitalWrite(RT_DIGIT_SET_VALUE, LOW);
    digitalWrite(LT_DIGIT_SET_VALUE, LOW);
    digitalWrite(RT_DIGIT_TEMP_VALUE, LOW);
    digitalWrite(LT_DIGIT_TEMP_VALUE, LOW);
    digitalWrite(RT_DIGIT_HUM_VALUE, LOW);
    digitalWrite(LT_DIGIT_HUM_VALUE, LOW);
}


void DisplayDigits(int setValue, int actualValue, int tempValue) {

    for (int i = 2 ; i >= 0; i--)
    {
//        ResetDigits();

        delay(1);

        int value = setValue;
        int output1 = RT_DIGIT_SET_VALUE;
        int output2 = LT_DIGIT_SET_VALUE;

        if (i == 0) {
            value = setValue;
            output1 = RT_DIGIT_SET_VALUE;
            output2 = LT_DIGIT_SET_VALUE;
        }
        else if (i == 1) {
            value = tempValue;
            output1 = RT_DIGIT_TEMP_VALUE;
            output2 = LT_DIGIT_TEMP_VALUE;
        }
        else if (i == 2) {
            value = actualValue;
            output1 = RT_DIGIT_HUM_VALUE;
            output2 = LT_DIGIT_HUM_VALUE;
        }

        int rightDigit = value % 10;
        int leftDigit = (value - rightDigit) / 10;

        if (value < -1) {
            rightDigit = -5;
            leftDigit = -5;
        }

        delay(1);

        // turns on RIGHT digit
        digitalWrite(output1, HIGH);
        digitalWrite(output2, LOW);

        DisplayDigit(rightDigit);

        delay(1);

        // turns on LEFT digit
        digitalWrite(output1, LOW);
        digitalWrite(output2, HIGH);

        DisplayDigit(leftDigit);

        delay(1);

        digitalWrite(output1, LOW);
        digitalWrite(output2, LOW);

        delay(0.5);
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

    pinMode(RT_DIGIT_SET_VALUE, OUTPUT);
    pinMode(LT_DIGIT_SET_VALUE, OUTPUT);

    pinMode(RT_DIGIT_HUM_VALUE, OUTPUT);
    pinMode(LT_DIGIT_HUM_VALUE, OUTPUT);

    pinMode(RT_DIGIT_TEMP_VALUE, OUTPUT);
    pinMode(LT_DIGIT_TEMP_VALUE, OUTPUT);

    pinMode(6, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(19, OUTPUT);
    pinMode(26, OUTPUT);

    DisplayDigits(-5, -5, -5);

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

    while (true) {

        // update potentiometer outside of timer loop. Loop is only for measurements!
        setHumidity = GetHumiditySetLevel() + 0.0;

        DisplayDigits(setHumidity, avgHumidity, avgTemp);

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

            // TODO display avg humidity on 7-seg
            // TODO display avg temp on 7-seg

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


