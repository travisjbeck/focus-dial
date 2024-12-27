#include "Config.h"
#include "controllers/NetworkController.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <BluetoothA2DPSink.h>
#include <esp_bt.h>

NetworkController *NetworkController::instance = nullptr;

NetworkController::NetworkController()
    : a2dp_sink(),
      btPaired(false),
      bluetoothActive(false),
      bluetoothAttempted(false),
      lastBluetoothtAttempt(0),
      bluetoothTaskHandle(nullptr),
      webhookQueue(nullptr),
      webhookTaskHandle(nullptr),
      provisioningMode(false)
{

    instance = this;
}

void NetworkController::begin()
{
    WiFiProvisionerSettings();

    if (isWiFiProvisioned())
    {
        Serial.println("Stored WiFi credentials found. Connecting...");
        wifiProvisioner.connectToWiFi();
    }

    // Load bluetooth paired state from nvs
    preferences.begin("network", true);
    btPaired = preferences.getBool("bt_paired", false);
    preferences.end();

    if (btPaired)
    {
        Serial.println("Previously paired with a device. Initializing Bluetooth.");
        initializeBluetooth(); // Initialize Bluetooth if previously paired
    }
    else
    {
        Serial.println("No previous Bluetooth pairing found. Skipping Bluetooth initialization.");
    }

    // Load Webhook URL from NVS under the "focusdial" namespace
    preferences.begin("focusdial", true);
    webhookURL = preferences.getString("webhook_url", "");
    preferences.end();

    if (!webhookURL.isEmpty())
    {
        Serial.println("Loaded Webhook URL: " + webhookURL);
    }

    if (webhookQueue == nullptr)
    {
        webhookQueue = xQueueCreate(5, sizeof(char *));
    }

    if (webhookTaskHandle == nullptr)
    {
        xTaskCreatePinnedToCore(webhookTask, "Webhook Task", 4096, this, 0, &webhookTaskHandle, 1);
        Serial.println("Persistent webhook task started.");
    }
}

void NetworkController::update()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();
    }
}

bool NetworkController::isWiFiProvisioned()
{
    // Check for stored WiFi credentials
    preferences.begin("network", true);
    String storedSSID = preferences.getString("ssid", "");
    preferences.end();

    return !storedSSID.isEmpty(); // Return true if credentials are found
}

bool NetworkController::isWiFiConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

bool NetworkController::isBluetoothPaired()
{
    return btPaired;
}

void NetworkController::startProvisioning()
{
    Serial.println("Starting provisioning mode...");
    btPaired = false;        // Reset paired state for new provisioning
    bluetoothActive = true;  // Enable Bluetooth for pairing
    provisioningMode = true; // Indicate we are in provisioning mode
    initializeBluetooth();
    wifiProvisioner.setupAccessPointAndServer();
}

void NetworkController::stopProvisioning()
{
    Serial.println("Stopping provisioning mode...");
    bluetoothActive = false;  // Disable Bluetooth after provisioning
    provisioningMode = false; // Exit provisioning mode
    stopBluetooth();
}

void NetworkController::reset()
{
    wifiProvisioner.resetCredentials();
    if (btPaired)
    {
        a2dp_sink.clean_last_connection();
        saveBluetoothPairedState(false);
    }
    Serial.println("Reset complete. WiFi credentials and paired state cleared.");
}

void NetworkController::initializeBluetooth()
{
    if (bluetoothTaskHandle == nullptr)
    {

        // Configure the A2DP sink with empty callbacks to use it for the trigger only
        a2dp_sink.set_stream_reader(nullptr, false);
        a2dp_sink.set_raw_stream_reader(nullptr);
        a2dp_sink.set_on_volumechange(nullptr);
        a2dp_sink.set_avrc_connection_state_callback(nullptr);
        a2dp_sink.set_avrc_metadata_callback(nullptr);
        a2dp_sink.set_avrc_rn_playstatus_callback(nullptr);
        a2dp_sink.set_avrc_rn_track_change_callback(nullptr);
        a2dp_sink.set_avrc_rn_play_pos_callback(nullptr);
        a2dp_sink.set_spp_active(false);
        a2dp_sink.set_output_active(false);
        a2dp_sink.set_rssi_active(false);

        a2dp_sink.set_on_connection_state_changed(btConnectionStateCallback, this);

        Serial.println("Bluetooth A2DP Sink configured.");

        // Create task for handling Bluetooth
        xTaskCreate(bluetoothTask, "Bluetooth Task", 4096, this, 0, &bluetoothTaskHandle);
    }
}

void NetworkController::startBluetooth()
{
    if (btPaired)
    { // Only start if paired
        bluetoothActive = true;
    }
}

void NetworkController::stopBluetooth()
{
    bluetoothActive = false; // Stop Bluetooth activity
}

void NetworkController::btConnectionStateCallback(esp_a2d_connection_state_t state, void *obj)
{
    auto *self = static_cast<NetworkController *>(obj);

    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED)
    {
        Serial.println("Bluetooth device connected.");

        // Save paired state only in provisioning mode
        if (self->provisioningMode)
        {
            self->saveBluetoothPairedState(true);
            self->btPaired = true;
            Serial.println("Paired state saved during provisioning.");
        }
    }
    else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
    {
        Serial.println("Bluetooth device disconnected.");
        // No need to set flags; task loop will handle reconnection logic based on is_connected()
    }
}

void NetworkController::saveBluetoothPairedState(bool paired)
{
    preferences.begin("network", false);
    preferences.putBool("bt_paired", paired);
    preferences.end();
    btPaired = paired;
    Serial.println("Bluetooth pairing state saved in NVS.");
}

void NetworkController::bluetoothTask(void *param)
{
    NetworkController *self = static_cast<NetworkController *>(param);

    while (true)
    {
        // If in provisioning mode, start Bluetooth only once
        if (self->provisioningMode)
        {
            if (!self->bluetoothAttempted)
            {
                Serial.println("Starting Bluetooth for provisioning...");
                self->a2dp_sink.start("Focus Dial", true);
                self->bluetoothAttempted = true; // Mark as attempted to prevent repeated starts
            }
        }
        else
        {
            // Normal operation mode
            if (self->bluetoothActive && !self->bluetoothAttempted)
            {
                Serial.println("Starting Bluetooth...");
                self->a2dp_sink.start("Focus Dial", true); // Auto-reconnect enabled
                self->bluetoothAttempted = true;
                self->lastBluetoothtAttempt = millis(); // Record the time of the start attempt
            }

            // If Bluetooth is active but not connected, attempt reconnect every 2 seconds
            if (self->bluetoothActive && !self->a2dp_sink.is_connected() && (millis() - self->lastBluetoothtAttempt >= 2000))
            {
                Serial.println("Attempting Bluetooth reconnect...");
                self->a2dp_sink.start("Focus Dial", true);
                self->lastBluetoothtAttempt = millis(); // Update last attempt time
            }

            // If Bluetooth is not supposed to be active but is connected, disconnect
            if (!self->bluetoothActive && self->a2dp_sink.is_connected())
            {
                Serial.println("Stopping Bluetooth...");
                self->a2dp_sink.disconnect();
                self->bluetoothAttempted = false; // Allow re-attempt later
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void NetworkController::sendWebhookAction(const String &action)
{
    if (webhookQueue == nullptr)
    {
        webhookQueue = xQueueCreate(5, sizeof(char *));
    }

    char *actionCopy = strdup(action.c_str());
    if (actionCopy == nullptr)
    {
        Serial.println("Failed to allocate memory for webhook action.");
        return;
    }

    if (xQueueSend(webhookQueue, &actionCopy, 0) == pdPASS)
    {
        Serial.println("Webhook action enqueued: " + String(actionCopy));
    }
    else
    {
        Serial.println("Failed to enqueue webhook action: Queue is full.");
        free(actionCopy); // Free the memory if not enqueued
    }
}

void NetworkController::webhookTask(void *param)
{
    NetworkController *self = static_cast<NetworkController *>(param);
    char *action;

    while (true)
    {
        // Wait for a webhook action to arrive in the queue
        if (xQueueReceive(self->webhookQueue, &action, portMAX_DELAY) == pdPASS)
        {
            Serial.println("Processing webhook action: " + String(action));

            // Send the webhook request and check the response
            bool success = self->sendWebhookRequest(String(action));
            if (success)
            {
                Serial.println("Webhook action sent successfully.");
            }
            else
            {
                Serial.println("Failed to send webhook action.");
            }

            free(action); // Free the allocated memory for action

            Serial.println("Finished processing webhook action.");
        }

        // Small delay to yield
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

bool NetworkController::sendWebhookRequest(const String &action)
{
    if (webhookURL.isEmpty())
    {
        Serial.println("Webhook URL is not set. Cannot send action.");
        return false;
    }

    std::unique_ptr<WiFiClient> client;
    if (webhookURL.startsWith("https://"))
    {
        client.reset(new WiFiClientSecure());
        if (!client)
        {
            Serial.println("Memory allocation for WiFiClientSecure failed.");
            return false;
        }
        static_cast<WiFiClientSecure *>(client.get())->setInsecure(); // Not verifying server certificate
    }
    else
    {
        client.reset(new WiFiClient());
        if (!client)
        {
            Serial.println("Memory allocation for WiFiClient failed.");
            return false;
        }
    }

    HTTPClient http;
    bool result = false;

    if (http.begin(*client, webhookURL))
    {
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"action\":\"" + action + "\"}";

        // Send the POST request
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0)
        {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Response: " + response);
            result = true;
        }
        else
        {
            Serial.println("Error in sending POST: " + String(httpResponseCode));
        }

        http.end(); // Close the connection
    }
    else
    {
        Serial.println("Unable to connect to server.");
    }

    return result;
}

void NetworkController::WiFiProvisionerSettings()
{
    wifiProvisioner.enableSerialDebug(true);
    wifiProvisioner.AP_NAME = "Focus Dial";
    wifiProvisioner.SVG_LOGO =
        R"rawliteral(
        <svg width="297" height="135" viewBox="0 0 99 45" xmlns="http://www.w3.org/2000/svg" style="margin:1rem auto;">
            <g fill="currentColor">
                <path d="m54 15h3v3h-3z"/>
                <path d="m54 3h3v3h-3z"/>
                <path d="m60 9v3h-6v3h-3v6h-3v-6h-3v-3h-6v-3h6v-3h3v-6h3v6h3v3z"/>
                <path d="m42 3h3v3h-3z"/><path d="m42 15h3v3h-3z"/>
                <path d="m21 30v12h-3v-9h-3v-3z"/><path d="m18 42v3h-6v-12h3v9z"/>
                <path d="m84 33h3v12h-3z"/><path d="m48 33h3v3h6v6h-3v-3h-6z"/>
                <path d="m99 42v3h-9v-15h3v12z"/><path d="m27 42h6v3h-6z"/><path d="m36 30h3v12h-3z"/>
                <path d="m48 42h6v3h-6z"/><path d="m81 30h3v3h-3z"/><path d="m24 33h3v9h-3z"/><path d="m51 30h6v3h-6z"/>
                <path d="m39 42h3v3h-3z"/><path d="m0 33h3v3h6v3h-6v6h-3z"/><path d="m3 30h6v3h-6z"/><path d="m72 30h3v15h-3z"/>
                <path d="m42 30h3v12h-3z"/><path d="m66 33h3v9h-3z"/><path d="m78 33h3v12h-3z"/><path d="m63 42h3v3h-6v-15h6v3h-3z"/>
                <path d="m27 30h6v3h-6z"/>
            </g>
        </svg>
        <style> /* Override lib defaults */
            :root {
                --theme-color: #4caf50;    
                --font-color: #fff;
                --card-background: #171717;
                --black: #080808;
            }
            body {
                background-color: var(--black);
            }
            input {
                background-color: #2b2b2b;
            }
            .error input[type="text"],
            .error input[type="password"] {
                background-color: #3e0707;
            }
            input[type="text"]:disabled ,input[type="password"]:disabled ,input[type="radio"]:disabled {
                color:var(--black);
            }
        </style>)rawliteral";

    wifiProvisioner.HTML_TITLE = "Focus Dial - Provisioning";
    wifiProvisioner.PROJECT_TITLE = " Focus Dial — Setup";
    wifiProvisioner.PROJECT_INFO = R"rawliteral(
            1. Connect to Bluetooth if you want to use the phone automation trigger.
            2. Select a WiFi network to save and allow Focus Dial to trigger webhook automations.
            3. Enter the webhook URL below to trigger it when a focus session starts.)rawliteral";

    wifiProvisioner.FOOTER_INFO = R"rawliteral(
        Focus Dial - Made by <a href="https://youtube.com/@salimbenbouz" target="_blank">Salim Benbouziyane</a>)rawliteral";

    wifiProvisioner.CONNECTION_SUCCESSFUL =
        "Provision Complete. Focus Dial will now start and status led will turn to blue.";

    wifiProvisioner.RESET_CONFIRMATION_TEXT =
        "This will erase all settings and require re-provisioning. Confirm on the device.";

    wifiProvisioner.setShowInputField(true);
    wifiProvisioner.INPUT_TEXT = "Webhook URL to Trigger Automation:";
    wifiProvisioner.INPUT_PLACEHOLDER = "e.g., https://example.com/webhook";
    wifiProvisioner.INPUT_INVALID_LENGTH = "The URL appears incomplete. Please enter the valid URL to trigger the automation.";
    wifiProvisioner.INPUT_NOT_VALID = "The URL entered is not valid. Please verify it and try again.";

    // Set the static methods as callbacks
    wifiProvisioner.setInputCheckCallback(validateInputCallback);
    wifiProvisioner.setFactoryResetCallback(factoryResetCallback);
}

// Static method for input validation callback
bool NetworkController::validateInputCallback(const String &input)
{
    if (instance)
    {
        return instance->validateInput(input);
    }
    return false;
}

// Static method for factory reset callback
void NetworkController::factoryResetCallback()
{
    if (instance)
    {
        instance->handleFactoryReset();
    }
}

bool NetworkController::validateInput(const String &input)
{
    String modifiedInput = input;

    // Check if URL starts with "http://" or "https://"
    if (!(modifiedInput.startsWith("http://") || modifiedInput.startsWith("https://")))
    {
        // If none supplied assume "http://"
        modifiedInput = "http://" + modifiedInput;
        Serial.println("Protocol missing, defaulting to http://");
    }

    // Basic validation
    int protocolEnd = modifiedInput.indexOf("://") + 3;
    int dotPosition = modifiedInput.indexOf('.', protocolEnd);

    bool isValid = (dotPosition != -1);

    Serial.print("Validating input: ");
    Serial.println(modifiedInput);

    // Save URL to NVS here if valid
    if (isValid)
    {
        Serial.println("URL is valid. Saving to NVS...");

        if (preferences.begin("focusdial", false))
        { // false means open for writing
            preferences.putString("webhook_url", modifiedInput);
            preferences.end();
            webhookURL = modifiedInput;
            Serial.println("Webhook URL saved: " + webhookURL);
        }
        else
        {
            Serial.println("Failed to open NVS for writing.");
        }
    }
    else
    {
        Serial.println("Invalid URL. Not saving to NVS.");
    }

    return isValid;
}

void NetworkController::handleFactoryReset()
{
    Serial.println("Factory reset initiated.");
    reset();
}