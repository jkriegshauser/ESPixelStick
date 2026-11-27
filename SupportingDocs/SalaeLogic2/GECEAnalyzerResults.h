#pragma once

#include <AnalyzerResults.h>

class GECEAnalyzer;

class GECEAnalyzerResults : public AnalyzerResults
{
public:
    GECEAnalyzerResults(
        GECEAnalyzer* analyzer,
        AnalyzerSettings* settings);

    virtual ~GECEAnalyzerResults();

    virtual void GenerateBubbleText(
        U64 frame_index,
        Channel& channel,
        DisplayBase display_base);

    virtual void GenerateExportFile(
        const char* file,
        DisplayBase display_base,
        U32 export_type_user_id);

    virtual void GenerateFrameTabularText(
        U64 frame_index,
        DisplayBase display_base);

    virtual void GeneratePacketTabularText(
        U64 packet_id,
        DisplayBase display_base);

    virtual void GenerateTransactionTabularText(
        U64 transaction_id,
        DisplayBase display_base);

protected:
    GECEAnalyzer* mAnalyzer;
};
