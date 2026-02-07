
#ifndef EVCONTROLSYSTEM_ADS1115_HPP
#define EVCONTROLSYSTEM_ADS1115_HPP

#include <cstdint>
#include <string>
#include <vector>

class ADS1115
{
public:
    /*---System Types-------------------------------------------------------------------------------------------------*/
    enum class Mux {
        // Differential
        AIN0_AIN1 = 0,
        AIN0_AIN3 = 1,
        AIN1_AIN3 = 2,
        AIN2_AIN3 = 3,
        // Single-ended
        AIN0_GND = 4,
        AIN1_GND = 5,
        AIN2_GND = 6,
        AIN3_GND = 7
    };

    enum class PGA {
        FSR_6_144V = 0,  // ±6.144V
        FSR_4_096V = 1,  // ±4.096V
        FSR_2_048V = 2,  // ±2.048V
        FSR_1_024V = 3,  // ±1.024V
        FSR_0_512V = 4,  // ±0.512V
        FSR_0_256V = 5   // ±0.256V
    };

private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    std::string devicePath;
    
    Mux muxSetting;
    PGA pgaSetting;
    
    float voltageDividerR1;
    float voltageDividerR2;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    int16_t readRaw();
    float readScale();
    float digitalToVoltage();
    [[nodiscard]] float getDividerRatio() const;
    static float getPGAScale(PGA pga);
    std::vector<int> getChannels();

    /*--File helper methods-------------------------------------------------------------------------------------------*/
    static std::string buildPath(const std::vector<int>& ch);
    std::string inputConfig_path();
    int16_t readSysfsInt(const std::string& file) const;
    float readSysfsFloat(const std::string& file) const;
    void writeSysfsFloat(const std::string& file, float value) const;
    void writeSysfsInt(const std::string& file, int value) const;

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    explicit ADS1115(std::string  iioDevicePath = "/sys/bus/iio/devices/iio:device0", float r1 = 3000.0f, float r2 = 8200.0f);
    ~ADS1115() = default;

    /*---Configuration Methods----------------------------------------------------------------------------------------*/
    void setInputMux(Mux mux);
    void setPGA(PGA pga);
    void setVoltageDivider(float r1, float r2);

    /*---Read voltage from ADS1115------------------------------------------------------------------------------------*/
    float readBatteryVoltage();
};

#endif //EVCONTROLSYSTEM_ADS1115_HPP