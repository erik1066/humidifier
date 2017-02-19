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
#include <queue>
#include "timer.h"

using namespace std;

#ifndef TOLERANCE
#define TOLERANCE 2.0 // 2 degrees F
#endif

void TurnOnFan() {

}

void TurnOffFan() {

}

double GetAverage(double readings[7]) {
    double avg = 0.0;

    for (int i = 0; i < 7; i++) {
        avg = avg + readings[i];
    }

    return avg / 7;
}

void ShiftReadings(double readings[7]) {
    for (int i = 1; i < 7; i++) {
        readings[i - 1] = readings[i];
    }
}

double GetHumiditySettingFromUser() {
    return 50;
}

double GetCurrentTemperature() {
    return 72.5;
}

double GetCurrentHumidity() {
    return 50;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cout << "Initializing..." << endl;

    Timer t = Timer(5000); // wait 5 sec between readings

    t.Start();

    double pastTempReadings [7];
    double pastHumidityReadings [7];

    double curTemp = GetCurrentTemperature();
    double curHumidity = GetCurrentHumidity();

    for (int i = 0; i < 7; i++) {
        pastTempReadings[i] = curTemp;
        pastHumidityReadings[i] = curHumidity;
    }

    double setHumidity = 0.0;

    while (true) {

        // update potentiometer outside of timer loop. Loop is only for measurements!
        setHumidity = GetHumiditySettingFromUser();

        if (t.GetIsElapsed() == true) {
            // timer elapsed, so let's do a readings check and then reset the timer

            ShiftReadings(pastHumidityReadings);
            ShiftReadings(pastTempReadings);

            curHumidity = GetCurrentHumidity();
            curTemp = GetCurrentTemperature();

            pastHumidityReadings[7] = curHumidity;
            pastTempReadings[7] = curTemp;

            auto avgHumdity = GetAverage(pastHumidityReadings);
            auto avgTemp = GetAverage(pastTempReadings);

            // If the running average over past 7 readings is less than the SET value minus the allowable TOLERANCE, then turn on the fan
            if (avgHumdity < setHumidity - TOLERANCE) {
                TurnOnFan();
            }
            else {
                TurnOffFan();
            }

            t.Reset();
            t.Start();
        }
    }

    return a.exec();
}


