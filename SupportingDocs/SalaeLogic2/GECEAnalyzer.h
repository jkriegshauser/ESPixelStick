#pragma once

#include <Analyzer.h>
#include <AnalyzerChannelData.h>
#include <memory>

class GECEAnalyzerSettings;
class GECEAnalyzerResults;

class GECEAnalyzer : public Analyzer2
{
public:
    GECEAnalyzer();
    virtual ~GECEAnalyzer();

    virtual void SetupResults();

    virtual void WorkerThread();

    virtual bool NeedsRerun();

    U32 GenerateSimulationData(
        U64 minimum_sample_index,
        U32 device_sample_rate,
        SimulationChannelDescriptor** simulation_channels);

    double SamplesToUs(U64 samples);

protected:
    std::unique_ptr<GECEAnalyzerSettings> mSettings;
    std::unique_ptr<GECEAnalyzerResults> mResults;

    AnalyzerChannelData* mSerial;

    U32 mSampleRateHz;
};
