#include "AlarmController.h"
#include <Preferences.h>
#include "HWCDC.h"
#include <Wire.h>

extern "C" {
#include "es8311.h"
}

#include "esp_log.h"

extern HWCDC USBSerial;

// Global instance
AlarmController alarmController;

AlarmController::AlarmController() : 
    playing(false), 
    volume(50), 
    speakerEnabled(false),
    audioBuffer(nullptr),
    bufferSize(I2S_BUFFER_SIZE) {
}

AlarmController::~AlarmController() {
    end();
}

bool AlarmController::begin() {
    USBSerial.println("AlarmController: Initializing audio system");
    
    // Allocate audio buffer
    audioBuffer = (uint8_t*)malloc(bufferSize);
    if (!audioBuffer) {
        USBSerial.println("AlarmController: Failed to allocate audio buffer");
        return false;
    }
    
    // Initialize Power Amplifier pin
    pinMode(PA_ENABLE_PIN, OUTPUT);
    digitalWrite(PA_ENABLE_PIN, LOW);  // Start with PA disabled
    
    // Load saved volume from NVS
    Preferences prefs;
    if (prefs.begin("alarm", true)) {
        volume = prefs.getUChar("volume", 90);  // Default to 90% volume like example
        prefs.end();
    }
    
    // Initialize I2S first (like the working example)
    USBSerial.println("AlarmController: Initializing I2S");
    i2s.setPins(I2S_BCK_PIN, I2S_WS_PIN, I2S_DATA_IN_PIN, I2S_DATA_PIN, I2S_MCLK_PIN);
    if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
        USBSerial.println("AlarmController: Failed to initialize I2S");
        return false;
    }
    
    // Re-initialize Wire with ES8311 I2C pins (like example)
    Wire.begin(ES8311_SDA_PIN, ES8311_SCL_PIN);
    
    // Initialize ES8311 codec after I2S
    if (!initES8311()) {
        USBSerial.println("AlarmController: Failed to initialize ES8311 codec");
        i2s.end();
        return false;
    }
    
    // Check for sounds directory on SD card
    if (SD_MMC.exists("/sounds")) {
        USBSerial.println("AlarmController: /sounds directory found");
    } else {
        USBSerial.println("AlarmController: /sounds directory not found - will use default beep");
    }
    
    return true;
}

void AlarmController::end() {
    stopAlarm();
    
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
    
    i2s.end();
}


void AlarmController::enableSpeaker(bool enable) {
    if (enable && !speakerEnabled) {
        // Enable power amplifier via GPIO 46
        digitalWrite(PA_ENABLE_PIN, HIGH);
        delay(10);  // Allow power to stabilize
        speakerEnabled = true;
        USBSerial.println("AlarmController: Speaker enabled");
    } else if (!enable && speakerEnabled) {
        digitalWrite(PA_ENABLE_PIN, LOW);
        speakerEnabled = false;
        USBSerial.println("AlarmController: Speaker disabled");
    }
}

bool AlarmController::parseWavHeader(File& file, WavHeader& header) {
    if (file.read((uint8_t*)&header, sizeof(WavHeader)) != sizeof(WavHeader)) {
        return false;
    }
    
    // Validate WAV header
    if (memcmp(header.riff, "RIFF", 4) != 0 || 
        memcmp(header.wave, "WAVE", 4) != 0 ||
        memcmp(header.fmt, "fmt ", 4) != 0) {
        USBSerial.println("AlarmController: Invalid WAV file format");
        return false;
    }
    
    // Check if format is supported (PCM only)
    if (header.audioFormat != 1) {
        USBSerial.println("AlarmController: Only PCM WAV files supported");
        return false;
    }
    
    USBSerial.printf("AlarmController: WAV file - %d Hz, %d bit, %d channels\n", 
                     header.sampleRate, header.bitsPerSample, header.numChannels);
    
    // Find data chunk (some WAV files have extra chunks)
    char chunk[4];
    uint32_t chunkSize;
    while (file.available() >= 8) {
        file.read((uint8_t*)chunk, 4);
        file.read((uint8_t*)&chunkSize, 4);
        
        if (memcmp(chunk, "data", 4) == 0) {
            header.dataSize = chunkSize;
            return true;
        } else {
            // Skip this chunk
            file.seek(file.position() + chunkSize);
        }
    }
    
    return false;
}

bool AlarmController::playAlarm(const char* filename) {
    USBSerial.println("AlarmController: playAlarm() called");
    
    if (playing) {
        stopAlarm();
    }
    
    bool useDefaultSound = true;
    
    // Try to open file from SD card if filename provided
    if (filename && SD_MMC.exists(filename)) {
        String fullPath = String("/sounds/") + filename;
        if (SD_MMC.exists(fullPath)) {
            audioFile = SD_MMC.open(fullPath, FILE_READ);
            if (audioFile) {
                WavHeader header;
                if (parseWavHeader(audioFile, header)) {
                    useDefaultSound = false;
                    USBSerial.printf("AlarmController: Playing %s\n", filename);
                } else {
                    audioFile.close();
                }
            }
        }
    }
    
    // I2S is already initialized in begin()
    
    enableSpeaker(true);
    playing = true;
    
    // Use default sound if file not available
    if (useDefaultSound) {
        USBSerial.println("AlarmController: Using default alarm sound");
        // Create a task to play the alarm
        xTaskCreate([](void* param) {
            AlarmController* controller = (AlarmController*)param;
            controller->generateDefaultAlarm();
            vTaskDelete(NULL);
        }, "alarm_task", 4096, this, 1, NULL);
    }
    
    return true;
}

void AlarmController::stopAlarm() {
    if (playing) {
        playing = false;
        
        if (audioFile) {
            audioFile.close();
        }
        
        // Don't end I2S here - it's initialized in begin()
        enableSpeaker(false);
        
        USBSerial.println("AlarmController: Alarm stopped");
    }
}



void AlarmController::generateDefaultAlarm() {
    USBSerial.println("AlarmController: Generating default alarm sound");
    
    // Simple continuous beep like the example
    const int bufferSize = 1024;
    int16_t buffer[bufferSize];
    
    static float phase = 0;
    const float frequency = 1000.0; // 1kHz tone
    const float amplitude = 16383;  // Don't scale - ES8311 handles volume
    
    // Play for 3 seconds
    unsigned long startTime = millis();
    while (playing && (millis() - startTime < 3000)) {
        // Generate sine wave
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = (int16_t)(sin(phase) * amplitude);
            phase += 2.0 * PI * frequency / SAMPLE_RATE;
            if (phase >= 2.0 * PI) phase -= 2.0 * PI;
        }
        
        // Write to I2S
        i2s.write((uint8_t*)buffer, bufferSize * sizeof(int16_t));
        vTaskDelay(1);
    }
    
    // Stop after playing
    playing = false;
}

void AlarmController::setVolume(uint8_t vol) {
    volume = constrain(vol, 0, 100);
    
    // Save to NVS
    Preferences prefs;
    if (prefs.begin("alarm", false)) {
        prefs.putUChar("volume", volume);
        prefs.end();
    }
}

std::vector<String> AlarmController::listSounds() {
    std::vector<String> sounds;
    
    File dir = SD_MMC.open("/sounds");
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            String name = file.name();
            if (name.endsWith(".wav") || name.endsWith(".WAV")) {
                // Remove /sounds/ prefix if present
                if (name.startsWith("/sounds/")) {
                    name = name.substring(8);
                }
                sounds.push_back(name);
            }
            file = dir.openNextFile();
        }
    }
    
    return sounds;
}

bool AlarmController::soundExists(const char* filename) {
    String fullPath = String("/sounds/") + filename;
    return SD_MMC.exists(fullPath);
}

bool AlarmController::initES8311() {
    // Create ES8311 handle with I2C address 0 (0x18)
    es8311_handle_t es_handle = es8311_create(0, ES8311_ADDRRES_0);
    if (!es_handle) {
        USBSerial.println("AlarmController: Failed to create ES8311 handle");
        return false;
    }
    
    // Configure ES8311 clock
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = SAMPLE_RATE * 256,  // MCLK = 256 * fs
        .sample_frequency = SAMPLE_RATE
    };
    
    // Initialize ES8311
    esp_err_t ret = es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    if (ret != ESP_OK) {
        USBSerial.println("AlarmController: Failed to initialize ES8311");
        return false;
    }
    
    // Configure sample frequency
    ret = es8311_sample_frequency_config(es_handle, es_clk.mclk_frequency, es_clk.sample_frequency);
    if (ret != ESP_OK) {
        USBSerial.println("AlarmController: Failed to configure ES8311 sample frequency");
        return false;
    }
    
    // Configure for speaker output (disable microphone)
    ret = es8311_microphone_config(es_handle, false);
    if (ret != ESP_OK) {
        USBSerial.println("AlarmController: Failed to configure ES8311 microphone");
        return false;
    }
    
    // Set initial volume
    ret = es8311_voice_volume_set(es_handle, volume, NULL);
    if (ret != ESP_OK) {
        USBSerial.println("AlarmController: Failed to set ES8311 volume");
        return false;
    }
    
    // Set microphone gain (even though mic is disabled, example does this)
    ret = es8311_microphone_gain_set(es_handle, (es8311_mic_gain_t)3);
    if (ret != ESP_OK) {
        USBSerial.println("AlarmController: Failed to set ES8311 microphone gain");
        return false;
    }
    
    USBSerial.println("AlarmController: ES8311 codec initialized successfully");
    USBSerial.printf("AlarmController: Volume set to %d%%\n", volume);
    return true;
}