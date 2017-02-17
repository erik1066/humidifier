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

#include "timer.h"

Timer::Timer(milliseconds duration)
{
    _duration = duration;
    _running = false;
    _elapsed = milliseconds(0);
}

void Timer::Reset() {
    _startTime = std::chrono::system_clock::now();
    _elapsed = milliseconds(0);
}

void Timer::Start() {
    _startTime = std::chrono::system_clock::now();
    _running = true;
}

void Timer::Stop() {
    _running = false;
    _endTime = std::chrono::system_clock::now();
    auto d = _endTime - _startTime;
    milliseconds msec = duration_cast<milliseconds>(d);

    _elapsed += msec;
}

milliseconds Timer::GetRemaining() {

    if (_running == false)
    {
        milliseconds msec  = _duration - _elapsed;
        if (msec.count() < 0) { return milliseconds(0); }
        return msec;
    }
    else
    {
        auto d = std::chrono::system_clock::now() - _startTime;
        milliseconds msec = duration_cast<milliseconds>(d);
        milliseconds totalElapsed = msec + _elapsed;
        milliseconds diff = _duration - totalElapsed;
        if (diff.count() < 0) { return milliseconds(0); }
        return diff;
    }
}

bool Timer::GetIsElapsed() {
    if (_running == false)
    {
        milliseconds msec  = _duration - _elapsed;
        if (msec.count() < 0) return true;
        return false;
    }
    else
    {
        auto d = std::chrono::system_clock::now() - _startTime;
        milliseconds msec = duration_cast<milliseconds>(d);
        milliseconds totalElapsed = msec + _elapsed;
        milliseconds diff = _duration - totalElapsed;
        if (diff.count() < 0) { return true; }
        return false;
    }
}

bool Timer::GetIsRunning() {
    return _running;
}
