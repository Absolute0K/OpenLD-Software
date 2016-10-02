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
