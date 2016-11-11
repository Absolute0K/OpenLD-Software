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


#include <fftw3.h>
#include <math.h>

/* THIS IS A LIBRARY FOR DETECTING REM STAGES
 * INPUT: FILTERED SIGNAL (0.1hz to 35hz prefered) of FPZ-A2 (or FP1/2)
 * OUTPUT: 0 FOR NREM STAGE AND 1 FOR REM STAGE
 *
 * THIS PROGRAM IS BASED ON ALGORITHM PROPOSED BY - Imtiaz et al (DOI: 10.1007/s10439-014-1085-6)
 *
 * HOW TO USE THIS LIBRARY
    1. Initialize object:
        remDetect(Sampling Frequency, FFT size, Window size, Epoch in seconds - 120s pref)
    2. Set Analysis parameters:
        remDetect.set_limits(SEFd minimum, Absolute power maximum, Relative power min, Relative power max);
    3. Calculate magnitude spectrum:
        remDetect.fft_power_Spectrum(Filtered signal, FFT spectrum);
    4. Calculate values for each sub-epochs, defined as Window size:
        remDetect.calc_Epoch(FFT spectrum, 8, 16)
            -> 4a. IF THE ABOVE RETURNS 1:
                   EVALUATE REM_STAGE = remDetect.evaluate_REM_Epoch()
 *
 */

class remDetect {
public:
    remDetect(int Fs, int size_fft, int size_window, int epoch_in_sec);
    ~remDetect();

    // Routine for calculating spectrum
    void fft_power_Spectrum(double *data_input, double *spectrum_output);

    // Callable functions
    int calc_Epoch(double *spectrum, int f_Start, int f_End);
    int evaluate_REM_Epoch();
    void set_limits(double min_SEFd, double max_AP, double min_RP, double max_RP);
    int evaluate_EOG_REM_Epoch(double *EOG1, double *EOG2, double min_EOG);
    int evaluate_WAKE_Epoch(double *spectrum, int f_i1, int f_E1, float WAKE_THRESHOLD1, int f_i2, int f_E2, float WAKE_THRESHOLD2);

    // Output variables which are accessable
    double avg_SEFd, avg_RP, avg_AP, avg_EOG_IP;
    int epoch_Counter, rem_eog_Counter;

private:

    // Variables for FFT
    fftw_complex *fft_output;
    double       *fft_data;
    double       *hm_window;
    fftw_plan plan_spectrum;

    // Individual calculation routines for REM detection
    double SEF_sum(double *spectrum, int f_Start, int f_End);
    double SEFx(double *spectrum, int x, int f_Start, double sum_SEF);
    double absPower(double *spectrum, int f_Start, int f_End);
    double relPower(double *spectrum, int f_Start, int f_End);

    // Variables for REM detection
    double *SEFd, *RP, *AP;

    int m_size_window;
    int m_size_fft;
    int m_Fs;
    int m_Epoch;

    // Analysis parameters
    double m_min_SEFd, m_max_AP, m_min_RP, m_max_RP;

};
