#pragma once

#include <Arduino.h>
#include <vector>
#include <SD_MMC.h>
#include <FS.h>
#include "ESP_I2S.h"

// ESP32-S3-Touch-AMOLED-1.75 I2S Configuration for ES8311
#define I2S_MCLK_PIN    42    // Master Clock
#define I2S_BCK_PIN     9     // Bit Clock (BCLKPIN)
#define I2S_WS_PIN      45    // Word Select/LR Clock (WSPIN)
#define I2S_DATA_IN_PIN 8     // Data In (DIPIN)
#define I2S_DATA_PIN    10    // Data Out (DOPIN)
#define I2S_PORT        I2S_NUM_0
#define PA_ENABLE_PIN   46    // Power Amplifier Enable (PA)

// ES8311 I2C pins (from example code)
#define ES8311_SDA_PIN  15
#define ES8311_SCL_PIN  14

// Audio configuration
#define SAMPLE_RATE     16000  // 16kHz for alarm sounds (lower = less memory)
#define I2S_BUFFER_SIZE 512
#define I2S_BUFFER_COUNT 8

class AlarmController {
public:
    AlarmController();
    ~AlarmController();
    
    // Initialize I2S and speaker power
    bool begin();
    void end();
    
    // I2S instance
    I2SClass i2s;
    
    // Playback control
    bool playAlarm(const char* filename = nullptr);  // nullptr = default sound
    void stopAlarm();
    bool isPlaying() const { return playing; }
    
    // Volume control (0-100)
    void setVolume(uint8_t vol);
    uint8_t getVolume() const { return volume; }
    
    // Sound management
    std::vector<String> listSounds();
    bool soundExists(const char* filename);
    
    // Power management
    void enableSpeaker(bool enable);
    
private:
    bool playing;
    uint8_t volume;
    bool speakerEnabled;
    File audioFile;
    
    // WAV file header structure
    struct WavHeader {
        char riff[4];           // "RIFF"
        uint32_t fileSize;      // File size - 8
        char wave[4];           // "WAVE"
        char fmt[4];            // "fmt "
        uint32_t fmtSize;       // Format chunk size
        uint16_t audioFormat;   // Audio format (1 = PCM)
        uint16_t numChannels;   // Number of channels
        uint32_t sampleRate;    // Sample rate
        uint32_t byteRate;      // Byte rate
        uint16_t blockAlign;    // Block align
        uint16_t bitsPerSample; // Bits per sample
        char data[4];           // "data"
        uint32_t dataSize;      // Data chunk size
    };
    
    bool parseWavHeader(File& file, WavHeader& header);
    bool initES8311();
    
    // Default alarm sound (sine wave beep)
    void generateDefaultAlarm();
    
    // Audio buffer for processing
    uint8_t* audioBuffer;
    size_t bufferSize;
};

// Global instance
extern AlarmController alarmController;