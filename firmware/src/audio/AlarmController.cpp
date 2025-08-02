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
    
    // Override to 70% volume for testing
    volume = 70;
    
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
    
    // LittleFS should already be mounted by main firmware
    // Just check for sound files
    if (LittleFS.exists("/groove.wav")) {
        USBSerial.println("AlarmController: Sound files found in LittleFS");
        
        // List all WAV files
        std::vector<String> sounds = listSounds();
        if (sounds.size() > 0) {
            USBSerial.println("AlarmController: Available sounds:");
            for (const String& sound : sounds) {
                USBSerial.print("  - ");
                USBSerial.println(sound);
            }
        }
    } else {
        USBSerial.println("AlarmController: No sound files found - will use default beep");
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
    // Read the standard 44-byte WAV header
    if (file.read((uint8_t*)&header, sizeof(WavHeader)) != sizeof(WavHeader)) {
        USBSerial.println("AlarmController: Failed to read WAV header");
        return false;
    }
    
    // Validate WAV header
    if (memcmp(header.riff, "RIFF", 4) != 0 || 
        memcmp(header.wave, "WAVE", 4) != 0 ||
        memcmp(header.fmt, "fmt ", 4) != 0 ||
        memcmp(header.data, "data", 4) != 0) {
        USBSerial.println("AlarmController: Invalid WAV file format");
        USBSerial.printf("  RIFF: %.4s\n", header.riff);
        USBSerial.printf("  WAVE: %.4s\n", header.wave);
        USBSerial.printf("  fmt : %.4s\n", header.fmt);
        USBSerial.printf("  data: %.4s\n", header.data);
        return false;
    }
    
    // Check if format is supported (PCM only)
    if (header.audioFormat != 1) {
        USBSerial.println("AlarmController: Only PCM WAV files supported");
        return false;
    }
    
    USBSerial.printf("AlarmController: WAV file - %d Hz, %d bit, %d channels, %d bytes\n", 
                     header.sampleRate, header.bitsPerSample, header.numChannels, header.dataSize);
    
    return true;
}

bool AlarmController::playAlarm(const char* filename) {
    USBSerial.println("AlarmController: playAlarm() called");
    
    if (playing) {
        stopAlarm();
    }
    
    bool useDefaultSound = true;
    
    // Try to open file from LittleFS if filename provided
    if (filename) {
        String fullPath = String("/") + filename;
        USBSerial.printf("AlarmController: Looking for file: %s\n", fullPath.c_str());
        
        if (LittleFS.exists(fullPath)) {
            USBSerial.println("AlarmController: File exists in LittleFS");
            audioFile = LittleFS.open(fullPath, FILE_READ);
            if (audioFile) {
                USBSerial.printf("AlarmController: File opened, size: %d bytes\n", audioFile.size());
                WavHeader header;
                if (parseWavHeader(audioFile, header)) {
                    useDefaultSound = false;
                    USBSerial.printf("AlarmController: Playing %s from LittleFS\n", filename);
                } else {
                    USBSerial.println("AlarmController: Failed to parse WAV header");
                    audioFile.close();
                }
            } else {
                USBSerial.println("AlarmController: Failed to open file");
            }
        } else {
            USBSerial.println("AlarmController: File not found in LittleFS");
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
    } else {
        // Play WAV file from LittleFS
        xTaskCreate([](void* param) {
            AlarmController* controller = (AlarmController*)param;
            controller->playWavFile();
            vTaskDelete(NULL);
        }, "wav_task", 8192, this, 1, NULL);
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
    
    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            // Remove leading slash if present
            if (name.startsWith("/")) {
                name = name.substring(1);
            }
            if (name.endsWith(".wav") || name.endsWith(".WAV")) {
                sounds.push_back(name);
            }
            file = root.openNextFile();
        }
    }
    
    return sounds;
}

bool AlarmController::soundExists(const char* filename) {
    String fullPath = String("/") + filename;
    return LittleFS.exists(fullPath);
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
    
    // Set initial volume - ES8311 expects 0-100 range
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

void AlarmController::playWavFile() {
    USBSerial.println("AlarmController: Playing WAV file");
    
    if (!audioFile) {
        USBSerial.println("AlarmController: No audio file open");
        playing = false;
        return;
    }
    
    // WAV header was already parsed in playAlarm()
    // We're already at position 44 after reading the header
    // Audio data starts immediately after the header
    
    // Read and play audio data in chunks
    const size_t chunkSize = 4096;  // Read 4KB at a time
    uint8_t* buffer = (uint8_t*)malloc(chunkSize);
    
    if (!buffer) {
        USBSerial.println("AlarmController: Failed to allocate playback buffer");
        playing = false;
        audioFile.close();
        return;
    }
    
    unsigned long startTime = millis();
    size_t totalBytes = 0;
    
    while (playing && audioFile.available()) {
        size_t bytesRead = audioFile.read(buffer, chunkSize);
        if (bytesRead == 0) break;
        
        // Write to I2S
        size_t bytesWritten = i2s.write(buffer, bytesRead);
        totalBytes += bytesWritten;
        
        // Small delay to prevent buffer overrun
        vTaskDelay(1);
        
        // Safety timeout (30 seconds max)
        if (millis() - startTime > 30000) {
            USBSerial.println("AlarmController: Playback timeout");
            break;
        }
    }
    
    free(buffer);
    audioFile.close();
    
    unsigned long duration = millis() - startTime;
    USBSerial.printf("AlarmController: Playback complete - %d bytes in %lu ms\n", totalBytes, duration);
    
    // Stop after playing
    playing = false;
    enableSpeaker(false);
}