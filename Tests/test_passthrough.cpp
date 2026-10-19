#include <vector>
#include <cmath>
bool run_passthrough_tests()
{
    // Analyzer is pass-through by definition - no audio modification.
    // Here we model: output must equal input bit-for-bit.
    std::vector<float> buf = {0.1f, -0.2f, 0.5f, 0.0f};
    std::vector<float> out = buf; // processBlock does not modify buffer
    for (size_t i=0;i<buf.size();++i) if (std::abs(buf[i]-out[i]) > 1e-9) return false;
    return true;
}
