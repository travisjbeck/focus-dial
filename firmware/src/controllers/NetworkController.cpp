#include "Config.h"
#include "controllers/NetworkController.h"
#include "Controllers.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <BluetoothA2DPSink.h>
#include <esp_bt.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#include "controllers/LEDController.h"
#include "managers/ProjectManager.h"
#include "StateMachine.h"

NetworkController *NetworkController::instance = nullptr;

// Define WebSocket path
#define WS_PATH "/ws"

// Add explicit extern reference for ledController which is used in handleColorPreview
extern LEDController ledController;

NetworkController::NetworkController()
    : a2dp_sink(),
      _server(80),
      _webServerRunning(false),
      _ws(WS_PATH), // Initialize WebSocket with path
      _lastWsCleanupTime(0),
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
  Serial.println("NetworkController::begin() called.");
  WiFi.onEvent(_onWiFiEvent);

  WiFiProvisionerSettings();

  bool provisioned = isWiFiProvisioned();
  Serial.printf("isWiFiProvisioned() returned: %s\n", provisioned ? "true" : "false");

  if (provisioned)
  {
    Serial.println("Attempting WiFi connection (WiFi.begin())...");
    WiFi.begin();
  }
  else
  {
    Serial.println("No WiFi credentials stored. Skipping WiFi.begin().");
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
  preferences.begin("focusdial", true); // Start read-only
  webhookURL = preferences.getString("webhook_url", "");
  preferences.end();

  // --- Validate loaded URL ---
  bool urlInvalid = false;
  String lowerCaseUrl = webhookURL;
  lowerCaseUrl.toLowerCase();
  if (!webhookURL.isEmpty() && lowerCaseUrl.startsWith("http://https://"))
  {
    Serial.println("WARNING: Invalid 'http://HTTPS://' prefix found in stored webhook URL. Clearing.");
    urlInvalid = true;
  }
  // Add any other simple validation checks needed here (e.g., minimum length)
  // if (!webhookURL.isEmpty() && webhookURL.length() < 10) {
  //   Serial.println("WARNING: Stored webhook URL seems too short. Clearing.");
  //   urlInvalid = true;
  // }

  if (urlInvalid)
  {
    webhookURL = "";                       // Clear in memory
    preferences.begin("focusdial", false); // Re-open read-write
    preferences.remove("webhook_url");     // Remove from NVS
    preferences.end();
    Serial.println("Cleared invalid webhook URL from NVS.");
  }
  // --- End Validation ---

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
  if (_webServerRunning)
  {
    // This cleanup seems to be handled internally by ESPAsyncWebServer library
    // No explicit cleanup needed in loop usually.
  }

  // Periodically clean up WebSocket clients (every 30 seconds)
  if (millis() - _lastWsCleanupTime > 30000)
  {
    _cleanupWebSocketClients();
    _lastWsCleanupTime = millis();
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
  if (!isWiFiConnected() || webhookURL.isEmpty())
  {
    Serial.println("Webhook skipped: WiFi disconnected or URL not set.");
    return;
  }

  char *actionToSend = strdup(action.c_str());
  if (xQueueSend(webhookQueue, &actionToSend, (TickType_t)0) != pdPASS)
  {
    Serial.println("Failed to queue webhook action - queue full?");
    free(actionToSend);
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

    // Parse the action string (format: "action|projectName" or just "action")
    String actualAction = action;
    String projectName = "";
    int separatorIndex = action.indexOf('|');
    if (separatorIndex != -1)
    {
      actualAction = action.substring(0, separatorIndex);
      projectName = action.substring(separatorIndex + 1);
    }

    // Construct JSON payload
    JsonDocument doc;
    doc["action"] = actualAction;
    if (!projectName.isEmpty())
    {
      doc["project"] = projectName;
    }

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.printf("Sending webhook payload: %s\n", jsonPayload.c_str());

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
  // Tolerate leading/trailing whitespace
  String modifiedInput = input;
  modifiedInput.trim();

  // Don't save if input is empty after trimming
  if (modifiedInput.isEmpty())
  {
    Serial.println("Webhook URL is empty, clearing saved URL.");
    if (preferences.begin("focusdial", false))
    {
      preferences.remove("webhook_url");
      preferences.end();
      webhookURL = "";
    }
    return true; // Allow saving an empty URL
  }

  // Check if URL *already* starts with "http://" or "https://" (case-insensitive)
  String lowerCaseInput = modifiedInput;
  lowerCaseInput.toLowerCase(); // Modify in place
  bool hasProtocol = lowerCaseInput.startsWith("http://") || lowerCaseInput.startsWith("https://");

  if (!hasProtocol)
  {
    // If no protocol, assume "http://"
    modifiedInput = "http://" + modifiedInput;
    Serial.println("Protocol missing, defaulting to http://");
  }

  // Basic validation: check for :// and at least one dot after that.
  int protocolEnd = modifiedInput.indexOf("://");
  int dotPosition = -1;
  if (protocolEnd != -1)
  {
    dotPosition = modifiedInput.indexOf('.', protocolEnd + 3);
  }

  bool isValid = (protocolEnd != -1 && dotPosition != -1 && dotPosition > protocolEnd + 3);

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
  _stopWebServer();
  reset();
}

// --- Web Server Management ---

void NetworkController::_setupWebServerRoutes()
{
  Serial.println("_setupWebServerRoutes: Configuring routes...");

  // --- Define Specific API Routes FIRST ---

  // Setup WebSocket handler
  _ws.onEvent(std::bind(&NetworkController::_onWebSocketEvent, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4,
                        std::placeholders::_5, std::placeholders::_6));
  _server.addHandler(&_ws);
  Serial.println("WebSocket handler added at " WS_PATH);

  // Restore standard routes
  _server.on("/api/projects", HTTP_GET, std::bind(&NetworkController::handleGetProjects, this, std::placeholders::_1));
  Serial.println("Route registered: GET /api/projects");
  _server.on("/api/projects", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, std::bind(&NetworkController::handleAddProject, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  Serial.println("Route registered: POST /api/projects");

  // Add new route for UPDATE via POST
  _server.on("/api/updateProject", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, std::bind(&NetworkController::handleUpdateProjectPostRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  Serial.println("Route registered: POST /api/updateProject"); // New route

  // Route for DELETE via POST
  _server.on("/api/deleteProject", HTTP_POST, std::bind(&NetworkController::handleDeleteProjectPostRequest, this, std::placeholders::_1));
  Serial.println("Route registered: POST /api/deleteProject");

  // Webhook API endpoints
  _server.on("/api/webhook", HTTP_GET, std::bind(&NetworkController::handleGetWebhook, this, std::placeholders::_1));
  Serial.println("Route registered: GET /api/webhook");
  _server.on("/api/webhook", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, std::bind(&NetworkController::handleUpdateWebhook, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  Serial.println("Route registered: POST /api/webhook");

  // --- Then Serve Static Files ---
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(LittleFS, "/index.html", "text/html"); });
  Serial.println("Route registered: GET / (index.html)");
  _server.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html");
  Serial.println("Route registered: serveStatic('/')");

  // --- Not Found Handler (Must be Last) ---
  _server.onNotFound([](AsyncWebServerRequest *request)
                     {
        Serial.printf("Not Found: %s %s\n", request->methodToString(), request->url().c_str());
        request->send(404, "text/plain", "Not found"); });
  Serial.println("Route registered: onNotFound");
}

void NetworkController::_startWebServer()
{
  if (_webServerRunning)
    return; // Already running

  Serial.println("Initializing LittleFS...");
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    // Decide how to handle failure - maybe skip web server?
    return;
  }
  Serial.println("LittleFS mounted successfully.");

  Serial.println("Starting Web Server and mDNS...");

  _setupWebServerRoutes(); // Configure routes

  // Start mDNS
  if (MDNS.begin("focus-dial"))
  { // Hostname for .local access
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started: http://focus-dial.local");
  }
  else
  {
    Serial.println("Error starting mDNS");
  }

  _server.begin(); // Start the server
  _webServerRunning = true;
  Serial.println("Web Server started.");

  // Initialize WebSocket cleanup time
  _lastWsCleanupTime = millis();
}

void NetworkController::_stopWebServer()
{
  if (!_webServerRunning)
    return; // Already stopped

  Serial.println("Stopping Web Server and mDNS...");
  _server.end();
  MDNS.end();
  // No need to explicitly end LittleFS unless reformatting
  _webServerRunning = false;
  Serial.println("Web Server stopped.");
}

// Static WiFi Event Handler
// NOTE: This runs in a different context, avoid complex operations or blocking calls.
// Use the instance pointer carefully if needed for non-static member access.
void NetworkController::_onWiFiEvent(WiFiEvent_t event)
{
// Use Arduino-ESP32 events for clarity if available, otherwise system events
#ifdef ARDUINO_ARCH_ESP32
  Serial.print("[WiFi-event] event: ");
  Serial.println(WiFi.eventName(event));
#else
  Serial.printf("[WiFi-event] event: %d\n", event);
#endif

  switch (event)
  {
#ifdef ARDUINO_ARCH_ESP32
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
#else
  case SYSTEM_EVENT_STA_GOT_IP:
#endif
    Serial.println("WiFi connected (SYSTEM_EVENT_STA_GOT_IP)");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    if (instance)
    {
      Serial.println("Calling _startWebServer()...");
      instance->_startWebServer();
    }
    break;
#ifdef ARDUINO_ARCH_ESP32
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#else
  case SYSTEM_EVENT_STA_DISCONNECTED:
#endif
    Serial.println("WiFi lost connection (SYSTEM_EVENT_STA_DISCONNECTED)");
    if (instance)
    {
      Serial.println("Calling _stopWebServer()...");
      instance->_stopWebServer();
    }
    // Optional: Add a reconnect attempt here? Be careful of loops.
    // Serial.println("Attempting WiFi reconnect...");
    // WiFi.begin();
    break;
  default:
    // Optional: Log other events?
    // Serial.printf("Unhandled WiFi Event: %d\n", event);
    break;
  }
}

// --- API Handler Implementations ---

void NetworkController::handleGetProjects(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  const auto &projects = getProjectManagerInstance().getProjects();
  for (const auto &project : projects)
  {
    JsonObject obj = array.add<JsonObject>();
    obj["name"] = project.name;
    obj["color"] = project.color;
  }

  String responseJson;
  serializeJson(doc, responseJson);
  request->send(200, "application/json", responseJson);
}

void NetworkController::handleAddProject(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  if (index + len == total)
  { // Process only when the full body is received
    JsonDocument doc;
    JsonDocument filter;
    filter["name"] = true;
    filter["color"] = true;
    DeserializationError error = deserializeJson(doc, (const char *)data, len, DeserializationOption::Filter(filter));

    if (error)
    {
      Serial.printf("POST /api/projects JSON Error: %s\n", error.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    if (!doc.is<JsonObject>() || !doc["name"].is<const char *>() || !doc["color"].is<const char *>())
    {
      Serial.println("POST /api/projects Error: Missing or invalid fields");
      request->send(400, "application/json", "{\"error\":\"Missing or invalid 'name' or 'color' fields\"}");
      return;
    }

    Project newProject;
    newProject.name = doc["name"].as<String>();
    newProject.color = doc["color"].as<String>();

    if (getProjectManagerInstance().addProject(newProject))
    {
      JsonDocument responseDoc;
      JsonArray array = responseDoc.to<JsonArray>();
      const auto &projects = getProjectManagerInstance().getProjects();
      for (const auto &p : projects)
      {
        JsonObject obj = array.add<JsonObject>();
        obj["name"] = p.name;
        obj["color"] = p.color;
      }
      String responseJson;
      serializeJson(responseDoc, responseJson);
      request->send(201, "application/json", responseJson);
    }
    else
    {
      Serial.println("POST /api/projects Error: projectManager.addProject failed");
      request->send(400, "application/json", "{\"error\":\"Failed to add project (max reached or invalid data?)\"}");
    }
  }
}

void NetworkController::handleUpdateProjectPostRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  if (index + len == total)
  { // Process only when full body is received
    JsonDocument doc;
    // Filter for expected fields
    JsonDocument filter;
    filter["index"] = true;
    filter["name"] = true;
    filter["color"] = true;
    DeserializationError error = deserializeJson(doc, (const char *)data, len, DeserializationOption::Filter(filter));

    if (error)
    {
      Serial.printf("POST /api/updateProject JSON Error: %s\n", error.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    // Validate fields exist and have roughly expected types
    if (!doc.is<JsonObject>() ||
        !doc["index"].is<int>() ||
        !doc["name"].is<const char *>() ||
        !doc["color"].is<const char *>())
    {
      Serial.println("POST /api/updateProject Error: Missing or invalid fields");
      request->send(400, "application/json", "{\"error\":\"Missing or invalid 'index', 'name', or 'color' fields\"}");
      return;
    }

    int projectIndex = doc["index"].as<int>();
    Project updatedProject;
    updatedProject.name = doc["name"].as<String>();
    updatedProject.color = doc["color"].as<String>();

    Serial.printf("POST /api/updateProject Request for index: %d, Name: %s, Color: %s\n",
                  projectIndex, updatedProject.name.c_str(), updatedProject.color.c_str());

    if (getProjectManagerInstance().updateProject(projectIndex, updatedProject))
    {
      Serial.printf("Project %d updated successfully.\n", projectIndex);
      request->send(200, "application/json", "{\"message\":\"OK\"}");
    }
    else
    {
      Serial.printf("POST /api/updateProject Error: updateProject(%d) failed.\n", projectIndex);
      // Check if index was the reason for failure
      const auto &projects = getProjectManagerInstance().getProjects();
      if (projectIndex < 0 || projectIndex >= projects.size())
      {
        request->send(404, "application/json", "{\"error\":\"Project index not found\"}");
      }
      else
      {
        request->send(400, "application/json", "{\"error\":\"Failed to update project (invalid data?)\"}");
      }
    }
  }
}

void NetworkController::handleDeleteProjectPostRequest(AsyncWebServerRequest *request)
{
  int projectIndex = -1;
  // Read index from POST body parameters
  if (request->hasParam("index", true))
  { // true specifies to check POST parameters
    String indexStr = request->getParam("index", true)->value();
    projectIndex = indexStr.toInt();
  }
  else
  {
    Serial.println("POST /api/deleteProject Error: Missing 'index' parameter in body");
    request->send(400, "application/json", "{\"error\":\"Missing 'index' parameter in body\"}");
    return;
  }

  Serial.printf("POST /api/deleteProject Request for index: %d\n", projectIndex);

  const auto &projects = getProjectManagerInstance().getProjects();
  Serial.printf("Currently %d projects in list before delete\n", projects.size());
  for (size_t i = 0; i < projects.size(); i++)
  {
    Serial.printf("  Project[%d]: %s, %s\n", i, projects[i].name.c_str(), projects[i].color.c_str());
  }

  bool deleted = getProjectManagerInstance().deleteProject(projectIndex);
  Serial.printf("deleteProject returned: %s\n", deleted ? "true" : "false");

  const auto &updatedProjects = getProjectManagerInstance().getProjects();
  Serial.printf("Now %d projects in list after delete\n", updatedProjects.size());
  for (size_t i = 0; i < updatedProjects.size(); i++)
  {
    Serial.printf("  Project[%d]: %s, %s\n", i, updatedProjects[i].name.c_str(), updatedProjects[i].color.c_str());
  }

  if (deleted)
  {
    // Redirect back to the main page after successful deletion
    request->redirect("/");
  }
  else
  {
    Serial.printf("POST /api/deleteProject Error: Index %d not found\n", projectIndex);
    // Maybe return an error page or JSON instead of 404 if preferred
    request->send(404, "application/json", "{\"error\":\"Project index not found\"}");
  }
}

void NetworkController::handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void NetworkController::handleGetWebhook(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["url"] = webhookURL;

  String responseJson;
  serializeJson(doc, responseJson);
  request->send(200, "application/json", responseJson);
}

void NetworkController::handleUpdateWebhook(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  if (index + len == total)
  { // Process only when the full body is received
    JsonDocument doc;
    JsonDocument filter;
    filter["url"] = true;
    DeserializationError error = deserializeJson(doc, (const char *)data, len, DeserializationOption::Filter(filter));

    if (error)
    {
      Serial.printf("POST /api/webhook JSON Error: %s\n", error.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    if (!doc.is<JsonObject>() || !doc["url"].is<const char *>())
    {
      Serial.println("POST /api/webhook Error: Missing or invalid fields");
      request->send(400, "application/json", "{\"error\":\"Missing or invalid 'url' field\"}");
      return;
    }

    String newWebhookURL = doc["url"].as<String>();

    // Validate the URL
    if (validateInput(newWebhookURL))
    {
      // URL is valid and saved in validateInput method
      request->send(200, "application/json", "{\"message\":\"Webhook URL updated successfully\"}");
    }
    else
    {
      request->send(400, "application/json", "{\"error\":\"Invalid webhook URL format\"}");
    }
  }
}

// Implement WebSocket event handler
void NetworkController::_onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                          AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    // Reset color when a client disconnects
    handleColorReset();
    break;
  case WS_EVT_DATA:
    // Handle data from the WebSocket client
    if (len)
    {
      String message = String((char *)data, len);
      _handleWebSocketMessage(message, client->id());
    }
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

// Implement the WebSocket message handler
void NetworkController::_handleWebSocketMessage(const String &message, uint32_t clientId)
{
  Serial.printf("WebSocket message from client #%u: %s\n", clientId, message.c_str());

  // Parse the message format: "action:value"
  int separatorPos = message.indexOf(':');
  if (separatorPos == -1)
  {
    Serial.println("Invalid WebSocket message format");
    return;
  }

  String action = message.substring(0, separatorPos);
  String value = message.substring(separatorPos + 1);

  if (action == "preview-color")
  {
    // Check if the device is asleep
    if (stateMachine.getCurrentState() == &StateMachine::sleepState)
    {
      Serial.println("Device is asleep, waking up for color preview...");
      stateMachine.changeState(&StateMachine::idleState);
      // Small delay to allow state transition before setting color
      delay(50);
    }
    // Proceed with handling the color preview
    handleColorPreview(value);
  }
  else if (action == "reset-color")
  {
    // Handle reset, but don't wake the device just for reset
    handleColorReset();
  }
  else
  {
    Serial.printf("Unknown WebSocket action: %s\n", action.c_str());
  }
}

// Implement WebSocket client cleanup
void NetworkController::_cleanupWebSocketClients()
{
  _ws.cleanupClients();
  Serial.println("WebSocket clients cleaned up");
}

// Implement WebSocket broadcast
void NetworkController::_broadcastWebSocketMessage(const String &message)
{
  _ws.textAll(message);
}

// Implement color preview handler that interfaces with the StateMachine
void NetworkController::handleColorPreview(const String &hexColor)
{
  Serial.printf("Color preview requested: %s\n", hexColor.c_str());

  // Allow preview only if in IdleState (or just woken up to IdleState)
  if (stateMachine.getCurrentState() == &StateMachine::idleState)
  {
    // Use the ledController to update the LEDs using the preview methods
    ledController.setPreviewColor(hexColor); // This handles saving state and setting the solid color

    Serial.printf("LED color preview set to: %s\n", hexColor.c_str());
  }
  else
  {
    Serial.println("Color preview ignored - not in idle state");
  }
}

// Implement color reset handler
void NetworkController::handleColorReset()
{
  Serial.println("Color reset requested");

  // Reset preview mode via LEDController (this handles restoring the previous state)
  // No need to check state here, resetPreviewColor handles its own logic
  ledController.resetPreviewColor();

  // If we *were* in IdleState, ensure its default pattern is restored (redundant check but safe)
  if (stateMachine.getCurrentState() == &StateMachine::idleState)
  {
    stateMachine.resetLEDColor(); // Calls idleState.restoreDefaultLEDPattern()
    Serial.println("LED color reset to default IdleState pattern");
  }
  else
  {
    Serial.println("LED color preview reset (was not in Idle)");
  }
}