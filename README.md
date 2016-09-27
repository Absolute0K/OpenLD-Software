## Software that recieves data from OpenLD

### OpenLD
 - Saves **24 bit** EEG/EOG/ECG data to BDF file
 - Can configure # of channels + individual *channel labels*
 - Allows user to **configure ADS1299** by accessing SETTING_MODE in OpenLD through Bluetooth SPP
 - **Real time Impedence Monitor** - Does not work with active electrodes! Only passive electrodes. 
 - **REM detection with various Lucid Dream stimulus options**
 - Note, this program should be used in conjonction with the OpenLD hardware. Other EEG hardware platforms, such as the OpenBCI, will be supported in the future
 - **Make sure to visit: https://hackaday.io/project/13285 for more information: Build Instructions, Project logs, FAQ, etc.**

### Libraries
 - **remDetect:** A library that uses REM sleep stage detection algorithm, developped by [Imtiaz et al.](http://www.ncbi.nlm.nih.gov/pmc/articles/PMC4204008/)
 - **filterIIR:** A very rough IIR filter library, adapted from [Iowa Hills Software](http://www.iowahills.com/Index.html) IIR Biquad II code. All credit for the IIR code goes to them.

 - **Required libraries for this program:**
  - FFTW: For spectral analysis, used in remDetect library
  - nCurses: For console-GUI interface of the software
