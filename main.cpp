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
#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "timer.h"

using namespace std;

#ifndef TOLERANCE
#define TOLERANCE 2.0 // 2 degrees F
#endif

#define RELAY_PIN 14 // the 5v relay module is on this GPIO pin, it controls the fan
#define DHTPIN 16 // the pin for the temp/humidity sensor
#define MAXTIMINGS 85
#define MAXHISTORICAL 14

bool _fanOn = false;

void TurnOnFan() {
    digitalWrite(RELAY_PIN, LOW);
    cout << "Humidifier fan on" << endl;
    _fanOn = true;
}

void TurnOffFan() {
    digitalWrite(RELAY_PIN, HIGH);
    cout << "Humidifier fan off" << endl;
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

double GetHumiditySettingFromUser() {
    return 50;
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

        printf( "Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
            dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );

        temp = f;
        humidity = dht11_dat[0];
        return true;
    }
    else
    {
        printf( "Bad data read from DHT11, skipping\n" );
        return false;
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

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH); // fan starts in OFF position!

    cout << "Relay PIN set" << endl;

    double pastTempReadings [MAXHISTORICAL];
    double pastHumidityReadings [MAXHISTORICAL];

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

    double setHumidity = 0.0;

    cout << "Starting timer..." << endl;
    Timer timer = Timer(2000);
    timer.Start();
    cout << "Timer set" << endl;

    cout << "Running control code..." << endl;

    while (true) {

        // update potentiometer outside of timer loop. Loop is only for measurements!
        setHumidity = GetHumiditySettingFromUser();

        delay(100);

        if (timer.GetIsElapsed() == true)
        {
            bool valid = GetCurrentReadings(curTemp, curHumidity);

            delay(20);

            if (valid) {
                ShiftReadings(pastHumidityReadings);
                ShiftReadings(pastTempReadings);

                pastHumidityReadings[MAXHISTORICAL - 1] = curHumidity;
                pastTempReadings[MAXHISTORICAL - 1] = curTemp;
            }

            auto avgHumdity = GetAverage(pastHumidityReadings);
            auto avgTemp = GetAverage(pastTempReadings);

            // TODO display avg humidity
            // TODO display avg temp

            // If the running average over past MAHISTORICAL readings is less than the SET value minus the allowable TOLERANCE, then turn on the fan
            if (avgHumdity < setHumidity - TOLERANCE)
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

            timer = Timer(2000);
            timer.Start();
        }
    }

    return a.exec();
}


