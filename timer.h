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

#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include <chrono>

using namespace std::chrono;

class Timer
{
private:
    bool _running;
    milliseconds _duration;
    std::chrono::milliseconds _elapsed;
    std::chrono::system_clock::time_point _startTime = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point _endTime = std::chrono::system_clock::now();
public:
    //Timer(milliseconds duration);
    Timer(int milliseconds);
    void Start();
    void Stop();
    milliseconds GetRemaining();
    bool GetIsElapsed();
    void Reset();
    bool GetIsRunning();
};

#endif // TIMER_H
