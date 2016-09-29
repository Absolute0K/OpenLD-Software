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

#include "serialmonitor.h"
#include "edflib.h"
#include <QtSerialPort/QSerialPort>
#include <iostream>
#include <QStringList>

// Formula for impedance calculation - FUDGE CALCULATION
#define IMP_CALC(x) (x * 1.5)/0.06

const int SMP_FREQ = 250;
const double ADS1299_SCALE = 0.02235174445530706111277;

const int DATA_WINDOW = SMP_FREQ;
const int REM_DATA_WINDOW = SMP_FREQ * 2;
const int FFT_WINDOW = 512;
const int EPOCH_SEC = 30;
const int REM_COUNTER_THRESHOLD = 0;
const int EOG_COUNTER_THRESHOLD = 2;

// TIME CONSTANTS
const int WINDOW_TRIGGER = 60 * 4;
const int BTN_TIME_WINDOW = 60 * 7;

// Index calculation
#define arr_REM(x, y) x + (y) * REM_DATA_WINDOW

// IIR Coefficients
extern double coeffs_hp[];
extern double coeffs_hp_EOG[];
extern double coeffs_lp[];

// Character definitions
const char CHAR_DATA = 'D';
const char CHAR_IMP = 'I';
const char CHAR_EOW = ';';
const char CHAR_EOTP = 0x17;
const char CHAR_BTN = 'B';


SerialMonitor::SerialMonitor(QObject *parent) :
    QObject(parent)
{

    DAQ = new QSerialPort(this);
    //    DAQ->setPortName("/dev/rfcomm0"); <- For Linux
    DAQ->setPortName("COM3");
    DAQ->setBaudRate(230400);
    DAQ->setDataBits(QSerialPort::Data8);
    DAQ->setParity(QSerialPort::NoParity);
    DAQ->setStopBits(QSerialPort::OneStop);
    DAQ->setFlowControl(QSerialPort::NoFlowControl);

    // Keep looping until a connection is established
    std::cout << "Wut trying to connect to: " << DAQ->portName().toLatin1().data()
              << " at " << DAQ->baudRate() << "bps...\n";

    while (!DAQ->open(QIODevice::ReadWrite)) std::cout << "FAILED!\n";
    std::cout << "Success!\n";
    std::cout << "============================\n";

    // Open analysis log file
    QString analysis_filename = QDateTime::currentDateTime().toString("'analysis_'yyyy-MM-dd'_T'hh-mm'.txt'");
    analysisfile.open(analysis_filename.toLatin1().data());

    // Open awake log file
    analysis_filename = QDateTime::currentDateTime().toString("'awake_'yyyy-MM-dd'_T'hh-mm'.txt'");
    awakefile.open(analysis_filename.toLatin1().data());

    // Initialize BDF file
    this->init_BDF_file();

    // Set analysis channel index
    channel_analysis = 1;
    std::cout << "============================\n";
    std::cout << "Which channel is used for REM analysis? (EEG, EOG1, EOG2)\n"
                 "Starting from Channel # - recommended 1\n>> ";
    std::cin >> channel_analysis;
    channel_analysis--;

    // Set timer limit threshold
    disabled_Time_Window = 3 * 60 * 60;
    std::cout << "============================\n";
    std::cout << "How long until the alarm activates? (-1 for test mode) - recommended 3 hours\n>> ";
    std::cin >> disabled_Time_Window;

    if (disabled_Time_Window < 0) alarm_demo = 1; // Set DEMO mode if less than zero (-1)
    else alarm_demo = 0;

    disabled_Time_Window *= 3600;

    std::getchar();

    // Zero out timers and counters
    DataCounter = 0;
    time_passed_sec = 0;
    stimulus_delay_on = -1;
    time_failsafe_btn = -1;
    counter_REM = STIM_DISABLED_NUMBER;
    flag_REM_Ready = 0;
    failsafe_on = 0;
    disable_REM = 0;

    // Clear out Impedance buffer
    for (int i = 0; i < 8; i++) impedanceBuffer[i] = 0;

    // Initialize filter and REM detection objects
    filter_hp = new filterIIR(coeffs_hp, 1);
    filter_hp_EOG = new filterIIR(coeffs_hp_EOG, 18);
    filter_lp = new filterIIR(coeffs_lp, 6);
    rem_analysis = new remDetect(SMP_FREQ, FFT_WINDOW, REM_DATA_WINDOW, EPOCH_SEC);
    signal_nf_buffer = new double[REM_DATA_WINDOW * 8];
    fft_spectrum = new double[FFT_WINDOW/2];

    // Set parameters
    rem_analysis->set_limits(4, 17, -15, -13);
    //    rem_analysis->set_limits(4, 19, -15, -13); // More sensitive

    // Set up mp3 alert
    rem_sound_Alert = new QSound("rem_alert.wav");
    rem_sound_Alert->setLoops(10);
    reminder_sound_Alert = new QSound("reminder_alert.wav");
//    heartbeat_Alert = new QSound("heartbeat_alert.wav");

    // Commence the serial connection
    connect(DAQ, SIGNAL(readyRead()), this, SLOT(writeToSettings()));
    DAQ->write(QByteArray("S", 1));

}

void SerialMonitor::writeToText()
{
    while (DAQ->canReadLine()) {
        QString IncomingData = DAQ->readLine();

        char First_Char = IncomingData.at(0).toLatin1();

        if (First_Char == CHAR_DATA) {
            IncomingData.remove(0, 1);

            // Split the raw data by whitespaces
            QStringList ProcessedData = IncomingData.split(" ");

            for (int i = 0; i < channels; i++) {
                ProcessedData[i].remove(" ");
                dataBuffer[DataCounter + i * SMP_FREQ] = (int) ProcessedData[i].toInt();
                //std::cout << "Convert: " << ProcessedData[i].toLatin1().data() << " to " << DataBuffer[DataCounter] << std::endl;
            }

            DataCounter++;

        } else if (First_Char == CHAR_IMP && impedance_on) {
            IncomingData.remove(0, 1);

            // Split the raw data by whitespaces
            QStringList ProcessedData = IncomingData.split(" ");

            //std::cout << "Impedance values: ";

            for (int i = 0; i < channels; i++) {
                ProcessedData[i].remove(" ");
                impedanceBuffer[i] = (int) IMP_CALC(ProcessedData[i].toInt());
            }

            m_guiConsole->update_Impedance(impedanceBuffer, 8);

        } else if (First_Char == CHAR_BTN) {
            IncomingData.remove(0, 1);

            if (IncomingData.toInt()) {
                m_guiConsole->update_Toolbar(QString("Manual Override! Probably awake? Yeah: Delayed by %1 minutes").arg(BTN_TIME_WINDOW / 60));
                awakefile << (long) time_passed_sec << ", " << 2 << std::endl;

                // Failsafe is enabled only once after the alarm is engaged
                if (failsafe_on) {
                    time_failsafe_btn = time_passed_sec + STIM_DELAY_FAILSAFE;
                    failsafe_on = 0;
                } else time_failsafe_btn = -1;

                rem_sound_Alert->stop();
                reminder_sound_Alert->play();
//                heartbeat_Alert->play();

                // If Triggered in the WINDOW_TRIGGER duration, stop alarm for BTN_TIME_WINDOW minutes
                if (time_passed_sec <= disabled_Time_Window) disabled_Time_Window = time_passed_sec + BTN_TIME_WINDOW;

//                DAQ->write(QByteArray("O", 1));

                // Annotate
                edfwrite_annotation_latin1(BDFHandler, (long long)(time_passed_sec * 10000LL), (long long)( 1 * 10000LL), "TRIGGER");
            }

        } else if (First_Char == CHAR_EOW) {
            if (DataCounter != 0) analysisfile << "ERROR: Data Counter = " << DataCounter << " != 0" << std::endl;

            // Call analysis routine
            do_REM_Analysis();

            // Write to BDF
            for (int i = 0; i < channels; i++) edfwrite_digital_samples(BDFHandler, dataBuffer + SMP_FREQ * i);

            if (impedance_on) {
                for (int i = 0; i < channels; i++) edfwrite_digital_samples(BDFHandler, impedanceBuffer);
            }
        }

        // If we recieved DATA_WINDOW number of samples
        if (DataCounter == DATA_WINDOW){

            m_guiConsole->update_Time(QString("Tick tock Current Time: %1 (Elapsed Time: %2)")
                                      .arg(start_time.toString("hh:mm:ss"))
                                      .arg(QDateTime::fromTime_t(time_passed_sec).toUTC().toString("hh:mm:ss")));
            m_guiConsole->update_display();

            // Update time variables
            start_time = start_time.addSecs(1);
            time_passed_sec++;

            // Reset data counter
            DataCounter = 0;
        }
    }
}

SerialMonitor::~SerialMonitor()
{
    DAQ->close();
    edfclose_file(BDFHandler);
    m_guiConsole->~guiConsole();
    rem_analysis->~remDetect();
    analysisfile.close();
    awakefile.close();
}

void SerialMonitor::init_BDF_file()
{
    filename_BDF = QDateTime::currentDateTime().toString("'data_'yyyy-MM-dd'_T'hh-mm'.bdf'");

    channels = 1;
    std::cout << "How many channels are there? (Prepend '-' to disable Impedance Monitoring - Active Electrodes)\n>> ";
    std::cin >> channels;

    if (channels < 0) {
        impedance_on = 0;
        channels *= -1;
        BDFHandler = edfopen_file_writeonly(filename_BDF.toLatin1().data(), EDFLIB_FILETYPE_BDFPLUS, channels);

    } else {
        impedance_on = 1;
        BDFHandler = edfopen_file_writeonly(filename_BDF.toLatin1().data(), EDFLIB_FILETYPE_BDFPLUS, channels * 2);
    }

    std::cout << "Recording " << channels << " channels at " << SMP_FREQ << " SPS\n";
    dataBuffer = new int [channels * DATA_WINDOW]; // Allocate memory for the buffer


    // Channel setup for channels on data stream
    for (int i = 0; i < channels; i++){
        std::cout << "Enter Channel " << i + 1 << " Label: ";
        std::cin >> signalLabel[i];
        edf_set_label(BDFHandler, i, signalLabel[i]);
        edf_set_samplefrequency(BDFHandler, i, SMP_FREQ);
        // Range: +VREF = 4.5/24 * 1000000 and -VREF = -1 * +VREF * 2^23/(2^23 - 1)
        edf_set_physical_maximum(BDFHandler, i, (double) +187.5 * 1000.0);
        edf_set_physical_minimum(BDFHandler, i, (double) -187.5 * 1000.0 * 1.000000119);
        edf_set_digital_maximum(BDFHandler, i, 8388607);
        edf_set_digital_minimum(BDFHandler, i, -8388608);
        edf_set_physical_dimension(BDFHandler, i, "uV");
    }

    // Channel setup for channels on impedance logging
    if (impedance_on) {
        for (int i = channels; i < channels * 2; i++){
            strcat(signalLabel[i - channels], "_Impedance");
            std::cout << "Impedance label: " << signalLabel[i - channels] << "\n";
            edf_set_label(BDFHandler, i, signalLabel[i - channels]);
            edf_set_samplefrequency(BDFHandler, i, 1);
            // Unit is in ohms, max val
            edf_set_physical_maximum(BDFHandler, i, 1000.0);
            edf_set_physical_minimum(BDFHandler, i, 0.0);
            edf_set_digital_maximum(BDFHandler, i, 1000000);
            edf_set_digital_minimum(BDFHandler, i, 0);
            edf_set_physical_dimension(BDFHandler, i, "kOhms");
        }
    }

}

void SerialMonitor::do_REM_Analysis()
{

    double EEG[1000], EOG1[1000], EOG2[1000];

    if (flag_REM_Ready == 0) { // We only recieved 250 samples

        // Convert integer data into physical data - multiply by V_REF/(2^23 - 1)/PGA_GAIN
        for (int i = 0; i < DATA_WINDOW; i++) {
            signal_nf_buffer[arr_REM(i, channel_analysis)] =
                    ((double) dataBuffer[i + channel_analysis * SMP_FREQ]) * ADS1299_SCALE;
            signal_nf_buffer[arr_REM(i, channel_analysis + 1)] =
                    ((double) dataBuffer[i + (channel_analysis + 1) * SMP_FREQ]) * ADS1299_SCALE;
            signal_nf_buffer[arr_REM(i, channel_analysis + 2)] =
                    ((double) dataBuffer[i + (channel_analysis + 2) * SMP_FREQ]) * ADS1299_SCALE;
        }

        // Set flag_REM_Ready to 1 so the next time we can calculate subepoch
        flag_REM_Ready = 1;

    } else if (flag_REM_Ready == 1) { // We have recieved 500 samples now

        double output_filter1[REM_DATA_WINDOW];

        // Same as above, but appending this time
        for (int i = 0; i < DATA_WINDOW; i++) {
            signal_nf_buffer[arr_REM(i, channel_analysis) + REM_DATA_WINDOW/2] =
                    ((double) dataBuffer[i + channel_analysis * SMP_FREQ]) * ADS1299_SCALE;
            signal_nf_buffer[arr_REM(i, channel_analysis + 1) + REM_DATA_WINDOW/2] =
                    ((double) dataBuffer[i + (channel_analysis + 1) * SMP_FREQ]) * ADS1299_SCALE;
            signal_nf_buffer[arr_REM(i, channel_analysis + 2) + REM_DATA_WINDOW/2] =
                    ((double) dataBuffer[i + (channel_analysis + 2) * SMP_FREQ]) * ADS1299_SCALE;

        }

        // reset flag_REM_Ready
        flag_REM_Ready = 0;

        // Filter the signal buffer of 500 samples
        filter_hp->filtfilt(signal_nf_buffer, output_filter1, REM_DATA_WINDOW);
        filter_lp->filtfilt(output_filter1, EEG, REM_DATA_WINDOW);

        filter_hp_EOG->filtfilt(signal_nf_buffer + arr_REM(0, channel_analysis + 1), output_filter1, REM_DATA_WINDOW);
        filter_lp->filtfilt(output_filter1, EOG1, REM_DATA_WINDOW);

        filter_hp_EOG->filtfilt(signal_nf_buffer + arr_REM(0, channel_analysis + 2), output_filter1, REM_DATA_WINDOW);
        filter_lp->filtfilt(output_filter1, EOG2, REM_DATA_WINDOW);

        // Calculate magnitude spectrum
        rem_analysis->fft_power_Spectrum(EEG, fft_spectrum);

        // Evaluate REM for each data window
        // rem_analysis->evaluate_EOG_REM_Epoch(EOG1, EOG2, 1000); // Not sensitive
        rem_analysis->evaluate_EOG_REM_Epoch(EOG1, EOG2, 500); // Verry sensitive, 7 minute disable window very recommended

        // Calculate on each sub-epochs (REM_DATA_WINDOW) and when we reach the end of the epoch:
        if (rem_analysis->calc_Epoch(fft_spectrum, 8, 16)) {

            // Determine if I'm in REM stage or not
            stage_REM = rem_analysis->evaluate_REM_Epoch();

            // Save it to a file
            analysisfile << (long) time_passed_sec << ", "
                         << rem_analysis->avg_SEFd     << ", "
                         << rem_analysis->avg_RP       << ", "
                         << rem_analysis->avg_AP       << ", "
                         << rem_analysis->avg_EOG_IP   << ", "
                         << rem_analysis->rem_eog_Counter << ", "
                         << stage_REM << ", ";
            //   << std::endl; Don't end line YET

            // Update TUI output
            m_guiConsole->update_Config(QString("Analysis Timestamp: %1                    \n"
                                                "SEFd: %2  AP: %3  RP: %4  EOG: %5                 ")
                                        .arg(QDateTime::fromTime_t(time_passed_sec).toUTC().toString("hh:mm:ss"))
                                        .arg(QString::number(rem_analysis->avg_SEFd, 'f', 1))
                                        .arg(QString::number(rem_analysis->avg_AP, 'f', 1))
                                        .arg(QString::number(rem_analysis->avg_RP, 'f', 1))
                                        .arg(rem_analysis->rem_eog_Counter);

            // IF REM IS DETECTED INCREMENT counter_REM, if not, RESET  __OR__ alarm demo mode
            if (stage_REM || alarm_demo) {

                // If sufficient amount of time has passed, break time is over, EOG counter exceeds threshold, OR if demo is set every 5 minutes
                if ((time_passed_sec > disabled_Time_Window &&
                        rem_analysis->rem_eog_Counter > EOG_COUNTER_THRESHOLD)
                        || (alarm_demo && time_passed_sec > disabled_Time_Window && (time_passed_sec % (60 * 2) == 0))) {

                    // Do something
                    // Wait for 7 minutes so that it won't ring too frequently
                    //  disabled_Time_Window = time_passed_sec + BTN_TIME_WINDOW;
                    // Scratch that, only delay WHEN THE EXTERNAL BUTTON HAS BEEN TRIGGERED

                    // Bring the output low then high
                    DAQ->write(QByteArray("O", 1));

                    // Annotate BDF File
                    edfwrite_annotation_latin1(BDFHandler, (long long)((time_passed_sec - EPOCH_SEC) * 10000LL), (long long)( EPOCH_SEC * 10000LL), "REM + ALARM");

                    // Write to file one for YES STIMULUS
                    analysisfile << 1;

                    m_guiConsole->update_Toolbar(QString("It is Time!!"));

                    // Play REM Alert mp3 to wake myself up!
                    rem_sound_Alert->play();

                    // The alarm is on, hence begin WINDOW_TRIGGER for the trigger
                    disabled_Time_Window = time_passed_sec + WINDOW_TRIGGER;

                    // failsafe is turned on - NOTE THIS IS A DILD FAILSAFE
                    failsafe_on = 1;

                    // Enable the awake-disable function
                    disable_REM = 1;

                } else { // NO ALARM
                    analysisfile << 0;

                    // Annotate BDF File
                    edfwrite_annotation_latin1(BDFHandler, (long long)(time_passed_sec * 10000LL), (long long)( EPOCH_SEC * 10000LL), "REM");

                }


            } else {
                // Write to file zero for NO ALARM
                analysisfile << 0;

                m_guiConsole->update_Toolbar(QString("Recording and Analyzing ..."));
            }

            // Reset REM EOG Counter
            rem_analysis->rem_eog_Counter = 0;

            // End line for the Analysis file
            analysisfile << std::endl;

        }
    }
}

void SerialMonitor::start_curses()
{
    m_guiConsole = new guiConsole();

    m_guiConsole->update_Config(QString("Recording %1 channels at %2 SPS...\n"
                                        "Connected to: %3 at %4 bps")
                                .arg(channels).arg(SMP_FREQ).arg(DAQ->portName()).arg(DAQ->baudRate()));
    m_guiConsole->update_Toolbar(QString("Recording and Analyzing ..."));



    m_guiConsole->touch_all();
    m_guiConsole->update_display();
}

void SerialMonitor::detectEOW()
{
    // Wait until END OF WINDOW character (;) is detected.
    // This is so that we can synchronize with the BioEXG
    while (DAQ->canReadLine()) {
        QString IncomingData = DAQ->readLine();

        if (IncomingData.at(0).toLatin1() == CHAR_EOW) {
            disconnect(DAQ, SIGNAL(readyRead()), 0, 0);
            connect(DAQ, SIGNAL(readyRead()), this, SLOT(writeToText()));

            // Current time
            start_time = QTime::currentTime();
        }
    }
}

void SerialMonitor::writeToSettings()
{

    while (DAQ->canReadLine()) {
        QString IncomingData = DAQ->readLine();

        char first_char = IncomingData.at(0).toLatin1();

        if (first_char == CHAR_EOTP) {
            std::cout << ">> ";
            char input_char[3] = {0};

            // Get 3 characters for total of 3 arguments
            std::cin >> input_char;
            DAQ->write(input_char, 1);
            DAQ->write(input_char + 1, 1);
            DAQ->write(input_char + 2, 1);
            DAQ->write("\n", 1);

        } else if (first_char == 'E') {
            // Connect to detectEOW for synchronization if E is detected
            disconnect(DAQ, SIGNAL(readyRead()), 0, 0);
            connect(DAQ, SIGNAL(readyRead()), this, SLOT(detectEOW()));
            std::cout << "EXIT + Connected to detectEOW; Starting curses...\n";

            // Test stimulus (if electrically connected)
            DAQ->write(QByteArray("O", 1));

            this->start_curses();

            break;

            // If the line neither DATA nor END OF PACKET, then print
        } else if (first_char != CHAR_DATA && first_char != CHAR_EOW
                   && first_char != CHAR_IMP && first_char != CHAR_BTN
                   && !(first_char >= '0' && first_char <= '9'))
            // std::cout << int(first_char) << ": " << IncomingData.toLatin1().data();
            std::cout << IncomingData.toLatin1().data();

    }
}
