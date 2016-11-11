/* MIT License

   Copyright (c) [2016] [Jae Choi]

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
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
