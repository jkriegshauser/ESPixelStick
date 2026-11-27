#include "GECEAnalyzerSettings.h"

GECEAnalyzerSettings::GECEAnalyzerSettings()
{
    mInputChannelInterface.reset(
        new AnalyzerSettingInterfaceChannel());

    mInputChannelInterface->SetTitleAndTooltip(
        "GECE Data",
        "GECE Data Line");

    mInputChannelInterface->SetChannel(
        mInputChannel);

    AddInterface(mInputChannelInterface.get());

    ClearChannels();
    AddChannel(mInputChannel, "GECE", false);
}

GECEAnalyzerSettings::~GECEAnalyzerSettings()
{
}

bool GECEAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel =
        mInputChannelInterface->GetChannel();

    ClearChannels();
    AddChannel(mInputChannel, "GECE", true);

    return true;
}

void GECEAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel(
        mInputChannel);
}

void GECEAnalyzerSettings::LoadSettings(const char *settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    text_archive >> mInputChannel;

    ClearChannels();
    AddChannel(mInputChannel, "GECE", true);

    UpdateInterfacesFromSettings();
}

const char *GECEAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannel;

    return SetReturnString(
        text_archive.GetString());
}
