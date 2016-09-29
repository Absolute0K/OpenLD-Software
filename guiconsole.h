/*
    This file is part of the project OpenLD.

    OpenLD is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenLD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenLD.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GUICONSOLE_H
#define GUICONSOLE_H
#include <ncursesw/ncurses.h>
// #include <ncurses.h>

class guiConsole
{
public:
    guiConsole();
    ~guiConsole();

    void update_display();
    void touch_all();

    void update_Impedance(int imp_values[], int channels);
    void update_Time(QString current_time);
    void update_Config(QString string_Config);
    void update_Toolbar(QString string_Status);

    WINDOW *display_Impedance;
    WINDOW *display_Duration;
    WINDOW *display_Configs;
};

#endif // GUICONSOLE_H
