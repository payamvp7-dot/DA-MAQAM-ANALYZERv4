#include "YinPitchDetector.h"
#include <cmath>

//==============================================================================
void YinPitchDetector::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    // window 2048 for 44.1k ~ 46ms, good trade-off latency vs low-freq resolution
    windowSize = juce::jmax(1024, maxBlockSize);
    if (windowSize > 4096) windowSize = 4096;
    yinBuffer.assign(windowSize / 2, 0.0f);
    inputBuffer.assign(windowSize, 0.0f);

    tauMin = (int) (sampleRate / maxFreq);
    tauMax = (int) (sampleRate / minFreq);
    tauMin = juce::jmax(4, tauMin);
    tauMax = juce::jmin((int) yinBuffer.size() - 2, tauMax);

    reset();
}

YinPitchDetector::Result YinPitchDetector::processBlock(const float* samples, int numSamples)
{
    Result r;

    // RMS for voiced detection and level meter
    double sumSq = 0.0;
    for (int i = 0; i < numSamples; ++i) sumSq += samples[i] * samples[i];
    double rms = std::sqrt(sumSq / juce::jmax(1, numSamples));
    r.rmsDb = rms > 1e-6 ? 20.0f * std::log10((float)rms) : -100.0f;

    // Breath/noise gate: need minimum level
    if (rms < 0.005) // ~ -46 dBFS
    {
        r.voiced = false;
        r.confidence = 0.0f;
        return r;
    }

    // Copy to internal buffer (zero pad if needed)
    int copyN = juce::jmin(numSamples, windowSize);
    std::copy(samples, samples + copyN, inputBuffer.begin());
    if (copyN < windowSize) std::fill(inputBuffer.begin() + copyN, inputBuffer.end(), 0.0f);

    cumulativeMeanNormalizedDifference(inputBuffer.data(), windowSize);

    // Absolute threshold search
    int tauEstimate = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (yinBuffer[tau] < threshold)
        {
            // find local minimum
            while (tau + 1 <= tauMax && yinBuffer[tau + 1] < yinBuffer[tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate == -1)
    {
        r.voiced = false;
        r.confidence = 0.0f;
        return r;
    }

    // Parabolic interpolation for sub-sample accuracy
    float betterTau = parabolicInterpolation(yinBuffer, tauEstimate);

    float freq = (float) sampleRate / betterTau;

    // Octave error mitigation: check if doubling/halving gives lower CMND
    // Only correct if confidence of alternative is significantly better
    if (tauEstimate * 2 <= tauMax)
    {
        float alt = yinBuffer[tauEstimate * 2];
        if (alt < yinBuffer[tauEstimate] * 0.6f && freq > 150)
        {
            // likely octave down error (detected double freq), keep original but lower confidence
        }
    }
    if (tauEstimate / 2 >= tauMin)
    {
        int halfTau = tauEstimate / 2;
        if (yinBuffer[halfTau] < threshold * 1.2f && freq < 300)
        {
            float halfFreq = (float) sampleRate / halfTau;
            // if half period also valid, prefer lower octave for vocal (avoid falsetto octave jump)
            if (halfFreq >= minFreq && halfFreq <= 500)
            {
                // do not auto-correct aggressively; just note
            }
        }
    }

    // Confidence = 1 - cmnd (inverted aperiodicity)
    float cmnd = yinBuffer[tauEstimate];
    r.confidence = juce::jlimit(0.0f, 1.0f, 1.0f - cmnd);
    r.frequencyHz = freq;
    // Voiced if confidence > 0.5 and plausible vocal range
    r.voiced = r.confidence > 0.45f && freq >= minFreq && freq <= maxFreq;

    // Additional breathiness check: if RMS low but voiced, downgrade
    if (r.rmsDb < -40.0f) r.voiced = false;

    return r;
}

void YinPitchDetector::cumulativeMeanNormalizedDifference(const float* samples, int numSamples)
{
    int half = (int) yinBuffer.size();
    // Step 1: difference function
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau < half; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < half; ++j)
        {
            float delta = samples[j] - samples[j + tau];
            sum += delta * delta;
        }
        yinBuffer[tau] = sum;
    }
    // Step 2: cumulative mean normalized
    float runningSum = 0.0f;
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau < half; ++tau)
    {
        runningSum += yinBuffer[tau];
        if (runningSum == 0) yinBuffer[tau] = 1.0f;
        else yinBuffer[tau] *= (float) tau / runningSum;
    }
}

float YinPitchDetector::parabolicInterpolation(const std::vector<float>& buf, int tau) const
{
    int x0 = tau < 1 ? tau : tau - 1;
    int x2 = tau + 1 >= (int) buf.size() ? tau : tau + 1;
    if (x0 == tau || x2 == tau) return (float) tau;

    float s0 = buf[x0];
    float s1 = buf[tau];
    float s2 = buf[x2];
    float denom = s0 + s2 - 2.0f * s1;
    if (denom == 0) return (float) tau;
    float shift = 0.5f * (s0 - s2) / denom;
    return (float) tau + shift;
}
