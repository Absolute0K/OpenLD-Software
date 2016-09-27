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

#include "remDetect.h"

// Max frequency(bandwidth) of the signal. Used to calculate Relative Power (RP)
const int f_Max = 35;

#define FREQ_TO_BIN(x, N_FFT, Fs) int(x * N_FFT/Fs)
#define BIN_TO_FREQ(x, N_FFT, Fs) int(x * Fs/N_FFT)


remDetect::remDetect(int Fs, int size_fft, int size_window, int epoch_in_sec)
{
    // Set private size values
    m_Fs = Fs;
    m_size_fft = size_fft;
    m_size_window = size_window;
    m_Epoch = epoch_in_sec * Fs / m_size_window;

    // Set counters to zero
    epoch_Counter = 0;
    rem_eog_Counter = 0;

    // Zero out average values
    avg_SEFd = 0;
    avg_RP = 0;
    avg_AP = 0;
    avg_EOG_IP = 0;

    // Initialize variables for FFT
    fft_output = (fftw_complex *)  fftw_malloc(sizeof(fftw_complex) * size_fft);
    fft_data = new double[size_fft];
    hm_window = new double[size_window];
    plan_spectrum = fftw_plan_dft_r2c_1d(size_fft, fft_data, fft_output, FFTW_MEASURE);

    // Initalize variables for REM analysis
    SEFd = new double[epoch_in_sec];
    RP = new double[epoch_in_sec];
    AP = new double[epoch_in_sec];

    // Initalize window function
    for (int i = 0; i < size_window; i++)
        hm_window[i] = 0.54 - (0.46 * cos(2 * M_PI * ((double) i / (((double) size_window - 1)))));
}

int remDetect::calc_Epoch(double *spectrum, int f_Start, int f_End)
{
    // Calculate SEFd
    double m_SEF_sum = SEF_sum(spectrum, f_Start, f_End);
    SEFd[epoch_Counter] = SEFx(spectrum, 95, f_Start, m_SEF_sum) - SEFx(spectrum, 50, f_Start, m_SEF_sum);

    // Calculate Absolute and Relative power
    RP[epoch_Counter] = relPower(spectrum, f_Start, f_End);
    AP[epoch_Counter] = absPower(spectrum, f_Start, f_End);

    // Increment epoch counter, which corresponds to m_size_window/m_Fs seconds passed
    epoch_Counter++;

    if (epoch_Counter == m_Epoch) {
        // Reset global average values
        epoch_Counter = 0;
        avg_SEFd = 0;
        avg_AP = 0;
        avg_RP = 0;

        // Calculate the final (averaged) values
        for (int i = 0; i < m_Epoch; i++) {
            avg_SEFd += SEFd[i];
            avg_AP   += AP[i];
            avg_RP   += RP[i];
        }

        avg_SEFd /= m_Epoch;
        avg_AP /= m_Epoch;
        avg_RP /= m_Epoch;

        return 1;

    } else {

        return 0; // The final values have not been calulated (yet)
    }
}

int remDetect::evaluate_REM_Epoch()
{
    // If the SEFd value is bigger than the specified minimum
    if (avg_SEFd > m_min_SEFd) {
        // If the averaged Absolute power is smaller than specified maximum
        if (avg_AP < m_max_AP) {
            // If the averaged Relative power is between the specified interval
            if (avg_RP > m_min_RP && avg_RP < m_max_RP) {
                // REM DETECTED! :D
                return 1;

            } else return 0;
        } else return 0;
    } else return 0;
}

void remDetect::set_limits(double min_SEFd, double max_AP, double min_RP, double max_RP)
{
    m_min_SEFd = min_SEFd;
    m_max_AP   = max_AP;
    m_min_RP   = min_RP;
    m_max_RP   = max_RP;
}

void remDetect::fft_power_Spectrum(double *data_input, double *spectrum_output)
{

    // Copy and window data
    for (int i = 0; i < m_size_window; i++) fft_data[i] = data_input[i] * hm_window[i];

    // Execute fft
    fftw_execute(plan_spectrum);

    for (int i = 0; i < m_size_fft / 2; i++) {
        spectrum_output[i] = sqrt(fft_output[i][0] * fft_output[i][0] + fft_output[i][1] * fft_output[i][1]);

        // Scale the power spectrum output (4 cause it is windowed)
        spectrum_output[i] /= (m_size_fft / 4);
    }
}

double remDetect::SEF_sum(double *spectrum, int f_Start, int f_End)
{
    double sum_fft = 0;

    // Sum spectral power from f_Start hz to f_End hz, automatically accounting for fft size
    for (int i = FREQ_TO_BIN(f_Start, m_size_fft, m_Fs); i <= FREQ_TO_BIN(f_End, m_size_fft, m_Fs); i++) {
        sum_fft += spectrum[i] * spectrum[i] * ((double)m_Fs) / ((double)m_size_fft);
        //std::cout << "sum_fft[" << i << "]: " << sum_fft << std::endl;
    }
    return sum_fft;
}

double remDetect::SEFx(double *spectrum, int x, int f_Start, double sum_SEF)
{
    double sum_fft = 0;
    int i;

    // Sum spectral power from f_Start hz to a percentage of sum_fft. Since the result is
    // is an integer in double form, round the value
    for (i = FREQ_TO_BIN(f_Start, m_size_fft, m_Fs); sum_fft < ((double)x) / 100.0 * sum_SEF; i++) {
        sum_fft += spectrum[i] * spectrum[i] * ((double)m_Fs) / ((double)m_size_fft);
        //std::cout << "sum_fft[" << i << "]: " << sum_fft << std::endl;
    }
    return (((double)i) * ((double)m_Fs) / ((double)m_size_fft));
}

double remDetect::absPower(double *spectrum, int f_Start, int f_End)
{
    double sum_fft = 0;

    // Sum spectral power from f_Start hz to f_End hz, automatically accounting for fft size
    // Then calculate log value
    for (int i = FREQ_TO_BIN(f_Start, m_size_fft, m_Fs); i <= FREQ_TO_BIN(f_End, m_size_fft, m_Fs); i++) {
        sum_fft += spectrum[i] * ((double)m_Fs) / ((double)m_size_fft);
        //std::cout << "sum_fft[" << i << "]: " << sum_fft << std::endl;
    }
    return ((double) 20.0 * log10(sum_fft));
}

double remDetect::relPower(double *spectrum, int f_Start, int f_End)
{
    double ratio_spectrum = 0;

    // Get the ratio of a frequency band (f_Start:f_End) and the entire frequency bandwidth
    ratio_spectrum = absPower(spectrum, f_Start, f_End) - absPower(spectrum, 1, f_Max);

    return ratio_spectrum;
}

int remDetect::evaluate_EOG_REM_Epoch(double *EOG1, double *EOG2, double min_EOG)
{
    // Resat accumulator
    avg_EOG_IP = 0;

    // Optimized algorithm - it is equivalent with += (EOG1 - EOG2)^2 - (EOG1 + EOG2)^2
    for (int i = 0; i < m_size_window; i++) {
        avg_EOG_IP += -1.0 * EOG1[i] * EOG2[i];
    }
    // Average
    avg_EOG_IP /= m_size_window;

    // if the value is within the limit window then return 1, else 0
    // Typical value is: min = 1000 and max = 7000
    if (avg_EOG_IP > min_EOG) {
        rem_eog_Counter++;
        return 1;
    } else return 0;
}

// DEPRECATED: evaluate_WAKE_Epoch
int remDetect::evaluate_WAKE_Epoch(double *spectrum, int f_i1, int f_E1, float WAKE_THRESHOLD1, int f_i2, int f_E2, float WAKE_THRESHOLD2)
{
    if (relPower(spectrum, f_i1, f_E1) > WAKE_THRESHOLD1 ||
        relPower(spectrum, f_i2, f_E2) > WAKE_THRESHOLD2) return 1;
    else return 0;
}


remDetect::~remDetect()
{

    fftw_destroy_plan(plan_spectrum);
    fftw_free(fft_output);
    delete hm_window;
    delete fft_data;

    delete SEFd;
    delete RP;
    delete AP;
}
