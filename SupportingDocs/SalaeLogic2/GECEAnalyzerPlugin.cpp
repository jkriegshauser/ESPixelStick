#include "GECEAnalyzer.h"
#include "GECEAnalyzerSettings.h"

Analyzer* GetAnalyzerName()
{
    return new GECEAnalyzer();
}

const char* GetAnalyzerDisplayName()
{
    return "GE Color Effects (GECE)";
}

const char* GetAnalyzerDescription()
{
    return "GE Color Effects Holiday Light Protocol";
}

Analyzer* CreateAnalyzer()
{
    return new GECEAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
    delete analyzer;
}
