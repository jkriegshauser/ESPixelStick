#pragma once

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class GECEAnalyzerSettings : public AnalyzerSettings
{
public:
    GECEAnalyzerSettings();
    virtual ~GECEAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char* settings);
    virtual const char* SaveSettings();

public:
    Channel mInputChannel;

protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
};
