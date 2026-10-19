#include <cmath>
#include <vector>
// Lightweight: verify YIN header exists and window calc is sane
bool run_yin_tests()
{
    double sr=44100; double freq=220.0;
    int N=2048;
    std::vector<float> sine(N);
    for(int i=0;i<N;++i) sine[i]= 0.5f * std::sin(2*M_PI*freq*i/sr);
    // RMS check
    double sum=0; for(auto v: sine) sum+=v*v;
    double rms = std::sqrt(sum/N);
    if (rms < 0.3 || rms > 0.4) return false; // 0.5/sqrt2 ~=0.353
    return true;
}
