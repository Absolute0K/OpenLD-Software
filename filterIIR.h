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

class filterIIR
{
public:
    filterIIR(double *iirCoeffs, int stages);

    double SectCalcForm2(int k, double x);
    void RunIIRBiquadForm2(double *Input, double *Output, int NumSigPts);
    void filtfilt(double *data, double *data_out, int filter_size);
    void reverse(double arr[], int count);

private:
    double *m_iirCoeffs;
    int m_stages;

    double buffer0[100], buffer1[100], buffer2[100];

 };
