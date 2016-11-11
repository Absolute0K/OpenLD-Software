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


/*
    IIR Biquad filter C code is adapted from: http://www.iowahills.com/Index.html
    All credits go to its creator: Iowa Hills Software
*/

#include "filterIIR.h"
#include <iostream>

/* IIR coefficient layout:
    => b0, b1, b2, a1, a2
       ...
*/

filterIIR::filterIIR(double *iirCoeffs, int stages)
{

    m_iirCoeffs = iirCoeffs;
    m_stages = stages;
    for (int j = 0; j < m_stages; j++) { // Init the shift registers.
        buffer0[j] = 0.0;
        buffer1[j] = 0.0;
        buffer2[j] = 0.0;
    }

}

// Form 2 Biquad Section Calc, called by RunIIRBiquadForm2.
double filterIIR::SectCalcForm2(int k, double x)
{
    double y;

    buffer0[k] = x - m_iirCoeffs[3 + k * 5] * buffer1[k] - m_iirCoeffs[4 + k * 5] * buffer2[k];
    y = m_iirCoeffs[0 + k * 5] * buffer0[k] + m_iirCoeffs[1 + k * 5] * buffer1[k] + m_iirCoeffs[2 + k * 5] * buffer2[k];

    // Shift the register values
    buffer2[k] = buffer1[k];
    buffer1[k] = buffer0[k];

    return (y);
}

// Form 2 Biquad
// This uses one set of shift registers, buffer0, buffer1, and buffer2 in the center.
void filterIIR::RunIIRBiquadForm2(double *Input, double *Output, int NumSigPts)
{
    double y;
    int j, k;

    for (j = 0; j < NumSigPts; j++) {
        y = SectCalcForm2(0, Input[j]);
        for (k = 1; k < m_stages; k++) {
            y = SectCalcForm2(k, y);
        }
        Output[j] = y;
    }
}


void filterIIR::filtfilt(double *data, double *data_out, int filter_size)
{
    double data_reversed[10000];

    reverse(data, filter_size);
    for (int i = 0; i < 20; i++) RunIIRBiquadForm2(data, data_reversed, filter_size);

    reverse(data_reversed, filter_size);
    for (int i = 0; i < 20; i++) RunIIRBiquadForm2(data_reversed, data_out, filter_size);
}

void filterIIR::reverse(double arr[], int count)
{
    double temp;
    for (int i = 0; i < count / 2; ++i) {
        temp = arr[i];
        arr[i] = arr[count - i - 1];
        arr[count - i - 1] = temp;
    }
}
