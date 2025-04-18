#pragma once

#include <BluetoothA2DPSink.h>
#include <WiFiProvisioner.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "ProjectData.h"

// Define reasonable default sizes for JSON documents used in API handlers
// Adjust these based on MAX_PROJECTS and expected name/color lengths
// Note: For stack-allocated JsonDocument (v7), size needs care.
// Use ArduinoJson Assistant: https://arduinojson.org/v7/assistant/
// const size_t JSON_DOC_SIZE_GET = JSON_ARRAY_SIZE(MAX_PROJECTS) + MAX_PROJECTS * JSON_OBJECT_SIZE(2) + 200; // Rough estimate
// const size_t JSON_DOC_SIZE_POST_PUT = JSON_OBJECT_SIZE(2) + 256;                                           // Rough estimate

class NetworkController
{
public:
  NetworkController();
  void begin();
  void update();
  void startProvisioning();
  void stopProvisioning();
  void reset();
  bool isWiFiProvisioned();
  bool isWiFiConnected();
  bool isBluetoothPaired();
  void initializeBluetooth();
  void startBluetooth();
  void stopBluetooth();
  void sendWebhookAction(const String &action, int durationSetMinutes, unsigned long actualElapsedSeconds);

  // New methods for WebSocket color preview
  void handleColorPreview(const String &hexColor);
  void handleColorReset();

  void startWebServer();

private:
  BluetoothA2DPSink a2dp_sink;
  Preferences preferences;
  WiFiProvisioner::WiFiProvisioner wifiProvisioner; // Instance of WiFiProvisioner
  AsyncWebServer _server;
  bool _webServerRunning;

  // WebSocket server
  AsyncWebSocket _ws;
  unsigned long _lastWsCleanupTime;

  String webhookURL;
  String apiKey; // Added to store API Key
  bool btPaired; // Paired state loaded from NVS
  bool bluetoothActive;
  bool bluetoothAttempted;
  bool provisioningMode;
  unsigned long lastBluetoothtAttempt;

  void WiFiProvisionerSettings();
  void saveBluetoothPairedState(bool paired);
  static void btConnectionStateCallback(esp_a2d_connection_state_t state, void *obj);

  // Web Server management
  void _setupWebServerRoutes();
  void _startWebServer();
  void _stopWebServer();
  static void _onWiFiEvent(WiFiEvent_t event);

  // WebSocket handlers
  void _onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
  void _handleWebSocketMessage(const String &message, uint32_t clientId);
  void _cleanupWebSocketClients();
  void _broadcastWebSocketMessage(const String &message);

  // --- API Route Handlers (Member Functions) ---
  void handleGetProjects(AsyncWebServerRequest *request);
  void handleAddProject(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
  void handleUpdateProject(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
  void handleDeleteProject(AsyncWebServerRequest *request);
  void handleApiOptions(AsyncWebServerRequest *request); // Common handler for OPTIONS
  void handleNotFound(AsyncWebServerRequest *request);
  void handleDeleteProjectPostRequest(AsyncWebServerRequest *request);
  void handleDeleteProjectByIdPostRequest(AsyncWebServerRequest *request);
  void handleUpdateProjectPostRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
  void handleGetWebhook(AsyncWebServerRequest *request);
  void handleUpdateWebhook(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
  void handleGetApiKeyStatus(AsyncWebServerRequest *request); // New handler for GET API Key status
  void handleUpdateApiKey(AsyncWebServerRequest *request);    // New handler for POST API Key

  // Tasks
  TaskHandle_t bluetoothTaskHandle;
  TaskHandle_t webhookTaskHandle;
  QueueHandle_t webhookQueue;

  static void bluetoothTask(void *param);
  static void webhookTask(void *param);
  bool sendWebhookRequest(const String &action);

  static NetworkController *instance;

  static bool validateInputCallback(const String &input);
  static void factoryResetCallback();

  bool validateInput(const String &input);
  void handleFactoryReset();
};
