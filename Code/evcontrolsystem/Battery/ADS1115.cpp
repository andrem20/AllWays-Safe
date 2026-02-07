#include "ADS1115.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <vector>
#include <string>

/*---Constructor/Destructor-------------------------------------------------------------------------------------------*/

ADS1115::ADS1115(std::string  iioDevicePath, float r1, float r2)
    : devicePath(std::move(iioDevicePath)),
      muxSetting(Mux::AIN0_GND),
      pgaSetting(PGA::FSR_6_144V),
      voltageDividerR1(r1),
      voltageDividerR2(r2)
{

    setInputMux(muxSetting);
    setPGA(pgaSetting);
    setVoltageDivider(voltageDividerR1, voltageDividerR2);

}

/*---Configuration Methods----------------------------------------------------------------------------------------*/

void ADS1115::setInputMux(Mux mux)
{
    muxSetting = mux;
}

void ADS1115::setPGA(PGA pga)
{
    pgaSetting = pga;
    writeSysfsFloat(inputConfig_path() + "_scale", getPGAScale(pgaSetting));
}

void ADS1115::setVoltageDivider(float r1, float r2)
{
    voltageDividerR1 = r1;
    voltageDividerR2 = r2;
}

/*---Read voltage from ADS1115------------------------------------------------------------------------------------*/
float ADS1115::readBatteryVoltage()
{
    float adcVoltage = digitalToVoltage();  //value read converted from digital to voltage after voltage divider
    return adcVoltage * getDividerRatio();  //return real voltage value
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

int16_t ADS1115::readRaw()
{
    return readSysfsInt(inputConfig_path() + "_raw");
}

float ADS1115::readScale()
{
    return readSysfsFloat(inputConfig_path() + "_scale");
}

float ADS1115::digitalToVoltage()
{
    const int16_t raw = readRaw();
    const float scale = readScale();
    return (static_cast<float>(raw) * scale) / 1000.0f;  // scale is in mV, convert to V
}

float ADS1115::getDividerRatio() const
{
    return (voltageDividerR1 + voltageDividerR2) / voltageDividerR2;
}

float ADS1115::getPGAScale(PGA pga)
{
    switch (pga)
    {
        case PGA::FSR_6_144V: return 0.1875f;
        case PGA::FSR_4_096V: return 0.125f;
        case PGA::FSR_2_048V: return 0.0625f;
        case PGA::FSR_1_024V: return 0.03125f;
        case PGA::FSR_0_512V: return 0.015625f;
        case PGA::FSR_0_256V: return 0.007813f;
        default: return 0.1875f;
    }
}

std::vector<int> ADS1115::getChannels()
{
    int m = static_cast<int>(muxSetting);

    if (m < 4)
    {
        // Differential channel mapping
        static constexpr int diff[4][2] = {
            {0, 1}, {0, 3}, {1, 3}, {2, 3}
        };
        return {diff[m][0], diff[m][1]};
    } else
        // Single-ended channel mapping (AINx_GND)
        return {m - 4};
}

/*--File helper methods-------------------------------------------------------------------------------------------*/

std::string ADS1115::buildPath(const std::vector<int>& ch)
{
    std::string path;

    if (ch.size() == 2)

        path += "in_voltage" + std::to_string(ch[0]) + "-voltage" + std::to_string(ch[1]);
    else
        path += "in_voltage" + std::to_string(ch[0]);

    return path;
}

std::string ADS1115::inputConfig_path()
{
    return buildPath(getChannels());
}

int16_t ADS1115::readSysfsInt(const std::string& file) const {
    std::ifstream f(devicePath + "/" + file);
    if (!f.is_open())
        throw std::runtime_error("Unable to open IIO sysfs file: " + file);

    int16_t val;
    f >> val;
    return val;
}

float ADS1115::readSysfsFloat(const std::string& file) const {
    std::ifstream f(devicePath + "/" + file);
    if (!f.is_open())
        throw std::runtime_error("Unable to open IIO sysfs file: " + file);

    float val;
    f >> val;
    return val;
}

void ADS1115::writeSysfsFloat(const std::string& file, float value) const {
    std::ofstream f(devicePath + "/" + file);
    if (!f.is_open())
        throw std::runtime_error("Unable to write to IIO sysfs file: " + file);

    f << value;
}

void ADS1115::writeSysfsInt(const std::string& file, int value) const {
    std::ofstream f(devicePath + "/" + file);
    if (!f.is_open())
        throw std::runtime_error("Unable to write to IIO sysfs file: " + file);

    f << value;
}
