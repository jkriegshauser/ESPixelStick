#include "GECEAnalyzer.h"
#include "GECEAnalyzerResults.h"
#include "GECEAnalyzerSettings.h"

#define BIT_COUNT 26

GECEAnalyzer::GECEAnalyzer()
: Analyzer2(),
  mSettings(new GECEAnalyzerSettings()),
  mResults(new GECEAnalyzerResults(this, mSettings.get()))
{
    SetAnalyzerSettings(mSettings.get());
}

GECEAnalyzer::~GECEAnalyzer()
{
    KillThread();
}

void GECEAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mSerial =
        GetAnalyzerChannelData(
            mSettings->mInputChannel);

    for (;;)
    {
        //
        // Wait for start pulse
        // GECE idle state is low
        //

        if (mSerial->GetBitState() == BIT_HIGH)
            mSerial->AdvanceToNextEdge();

        mSerial->AdvanceToNextEdge(); // rising

        U64 startHighBegin = mSerial->GetSampleNumber();

        mSerial->AdvanceToNextEdge(); // falling

        U64 startHighEnd = mSerial->GetSampleNumber();

        double startUs =
            SamplesToUs(
                startHighEnd - startHighBegin);

        if (startUs < 5.0 || startUs > 15.0)
            continue;

        U32 bits[BIT_COUNT];

        bool valid = true;

        for (U32 bit = 0; bit < BIT_COUNT; bit++)
        {
            U64 lowStart =
                mSerial->GetSampleNumber();

            mSerial->AdvanceToNextEdge();

            U64 lowEnd =
                mSerial->GetSampleNumber();

            double lowUs =
                SamplesToUs(lowEnd - lowStart);

            U64 highStart =
                lowEnd;

            mSerial->AdvanceToNextEdge();

            U64 highEnd =
                mSerial->GetSampleNumber();

            double highUs =
                SamplesToUs(highEnd - highStart);

            if (lowUs > highUs)
                bits[bit] = 1;
            else
                bits[bit] = 0;

            if (lowUs < 5 ||
                lowUs > 30 ||
                highUs < 5 ||
                highUs > 30)
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            continue;

        U32 address = 0;
        U32 brightness = 0;
        U32 blue = 0;
        U32 green = 0;
        U32 red = 0;

        for (int i = 0; i < 6; i++)
            address = (address << 1) | bits[i];

        for (int i = 6; i < 14; i++)
            brightness = (brightness << 1) | bits[i];

        for (int i = 14; i < 18; i++)
            blue = (blue << 1) | bits[i];

        for (int i = 18; i < 22; i++)
            green = (green << 1) | bits[i];

        for (int i = 22; i < 26; i++)
            red = (red << 1) | bits[i];

        Frame frame;

        frame.mStartingSampleInclusive =
            startHighBegin;

        frame.mEndingSampleInclusive =
            mSerial->GetSampleNumber();

        frame.mData1 = address;
        frame.mData2 =
            (brightness << 12) |
            (blue << 8) |
            (green << 4) |
            red;

        mResults->AddFrame(frame);
        mResults->CommitResults();

        ReportProgress(
            frame.mEndingSampleInclusive);
    }
}

double GECEAnalyzer::SamplesToUs(U64 samples)
{
    return (double(samples) * 1000000.0)
        / double(mSampleRateHz);
}

void GECEAnalyzer::SetupResults()
{
    SetAnalyzerResults(mResults.get());

    mResults->AddChannelBubblesWillAppearOn(
        mSettings->mInputChannel);
}

bool GECEAnalyzer::NeedsRerun()
{
    return false;
}

U32 GECEAnalyzer::GenerateSimulationData(
    U64 minimum_sample_index,
    U32 device_sample_rate,
    SimulationChannelDescriptor** simulation_channels)
{
    *simulation_channels = nullptr;
    return 0;
}
