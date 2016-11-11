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


#ifndef SERIALMONITOR_H
#define SERIALMONITOR_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include "guiconsole.h"
#include "filterIIR.h"
#include "remDetect.h"
#include <fstream>
#include <QDateTime>
#include <QSound>


class SerialMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SerialMonitor(QObject *parent = 0);
    ~SerialMonitor();

    // Make the GUI object public to main
    guiConsole *m_guiConsole;

    // BDF File init routine
    void init_BDF_file();
    //void init_SPP();

private:
    QSerialPort *DAQ;
    unsigned int DataCounter;

    // Buffers for raw data and impedance measurements
    int *dataBuffer;
    int impedanceBuffer[8];

    // Variables related to BDF files
    QString filename_BDF;
    int BDFHandler;
    int channels;
    char signalLabel[8][50];

    // IIR Filter object
    filterIIR *filter_hp;
    filterIIR *filter_hp_EOG;
    filterIIR *filter_lp;

    // REM detect object
    remDetect *rem_analysis;
    void do_REM_Analysis();

    int channel_analysis;
    int stage_REM;
    int flag_REM_Ready;

    int alarm_demo;
    int impedance_on;

    // uint32_t flag_Settings;
    // alarm_demo, impedance_on, flag_REM_Ready, failsafe_on
    int failsafe_on;

    long stimulus_delay_on;
    long time_failsafe_btn;

    double *signal_nf_buffer;
    double *fft_spectrum;

    // REM play sound file
    QSound *rem_sound_Alert;

    // File to save things in
    std::ofstream analysisfile;

    // Time variables
    long time_passed_sec;
    long disabled_Time_Window;
    QTime start_time;

    void start_curses();

private slots:
    void detectEOW();
    void writeToSettings();
    void writeToText();


};

#endif // SERIALMONITOR_H
