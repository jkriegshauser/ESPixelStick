#include "GECEAnalyzerResults.h"
#include "GECEAnalyzer.h"

GECEAnalyzerResults::GECEAnalyzerResults(
    GECEAnalyzer* analyzer,
    AnalyzerSettings* settings)
    : AnalyzerResults(),
      mAnalyzer(analyzer)
{
    SetAnalyzerSettings(settings);
}

GECEAnalyzerResults::~GECEAnalyzerResults()
{
}

void GECEAnalyzerResults::GenerateBubbleText(
    U64 frame_index,
    Channel&,
    DisplayBase)
{
    ClearResultStrings();

    Frame frame = GetFrame(frame_index);

    U32 addr = (U32)frame.mData1;

    U32 packed = (U32)frame.mData2;

    U32 bright =
        (packed >> 12) & 0xFF;

    U32 blue =
        (packed >> 8) & 0x0F;

    U32 green =
        (packed >> 4) & 0x0F;

    U32 red =
        packed & 0x0F;

    char text[128];

    sprintf(
        text,
        "A:%u B:%u R:%u G:%u Bl:%u",
        addr,
        bright,
        red,
        green,
        blue);

    AddResultString(text);
}

void GECEAnalyzerResults::GenerateFrameTabularText(
    U64 frame_index,
    DisplayBase display_base)
{
    ClearTabularText();
}

void GECEAnalyzerResults::GeneratePacketTabularText(
    U64 packet_id,
    DisplayBase display_base)
{
    ClearTabularText();
}

void GECEAnalyzerResults::GenerateTransactionTabularText(
    U64 transaction_id,
    DisplayBase display_base)
{
    ClearTabularText();
}

void GECEAnalyzerResults::GenerateExportFile(
    const char* file,
    DisplayBase display_base,
    U32 export_type_user_id)
{
}
