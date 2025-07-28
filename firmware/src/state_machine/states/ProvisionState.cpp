#include "ProvisionState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "../../ui/ScreenManager.h"
#include "HWCDC.h"

// External references
extern ScreenManager screenManager;
extern HWCDC USBSerial;

// DNS configuration for captive portal
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

ProvisionState::ProvisionState() : 
  apModeStarted(false),
  webServer(nullptr),
  dnsServer(nullptr),
  credentialsReceived(false),
  showingCredentialEntry(false)
{
}

ProvisionState::~ProvisionState()
{
  if (webServer) {
    delete webServer;
    webServer = nullptr;
  }
  if (dnsServer) {
    delete dnsServer;
    dnsServer = nullptr;
  }
}

void ProvisionState::onEnter()
{
  ESP_LOGI(getLogTag(), "Starting WiFi provisioning");
  USBSerial.println("[ProvisionState] Starting WiFi provisioning");
  
  apModeStarted = false;
  credentialsReceived = false;
  showingCredentialEntry = false;
  
  // Generate unique AP name
  deviceAPName = generateAPName();
  USBSerial.print("[ProvisionState] Generated AP name: ");
  USBSerial.println(deviceAPName);
  
  // Start AP mode
  startAPMode();
  
  // Show provisioning UI
  showProvisioningUI();
}

void ProvisionState::onUpdate()
{
  // Process DNS requests for captive portal
  if (dnsServer) {
    dnsServer->processNextRequest();
  }
  
  // Handle web server
  if (webServer) {
    webServer->handleClient();
  }
  
  // Check if credentials were received
  if (credentialsReceived) {
    ESP_LOGI(getLogTag(), "Credentials received, attempting connection");
    
    // Stop AP mode
    stopAPMode();
    
    // Try to connect
    if (connectToWiFi(ssid, password)) {
      ESP_LOGI(getLogTag(), "WiFi connection successful");
      
      // Save credentials
      saveCredentials(ssid, password);
      
      // Transition to idle state
      stateMachine.changeState(stateMachine.idleState);
    } else {
      ESP_LOGE(getLogTag(), "WiFi connection failed");
      
      // Restart AP mode
      credentialsReceived = false;
      startAPMode();
      updateProvisioningUI();
    }
  }
  
  // Handle touch input for manual entry
  if (showingCredentialEntry) {
    // TODO: Implement touch keyboard input
    // For now, rely on web interface
  }
  
  // TODO: Add swipe left gesture to cancel provisioning
  // For now, user can wait for timeout or connect via web interface
  
  yield();
}

void ProvisionState::onExit()
{
  ESP_LOGI(getLogTag(), "WiFi provisioning complete");
  
  // Stop AP mode if still running
  stopAPMode();
}

void ProvisionState::startAPMode()
{
  ESP_LOGI(getLogTag(), "Starting AP mode with SSID: %s", deviceAPName.c_str());
  USBSerial.print("[ProvisionState] Starting AP mode with SSID: ");
  USBSerial.println(deviceAPName);
  
  // Configure AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  bool success = WiFi.softAP(deviceAPName.c_str());
  USBSerial.print("[ProvisionState] WiFi.softAP() returned: ");
  USBSerial.println(success ? "true" : "false");
  
  // Start DNS server for captive portal
  dnsServer = new DNSServer();
  dnsServer->start(DNS_PORT, "*", apIP);
  
  // Start web server
  webServer = new WebServer(80);
  
  // Configure web server routes
  webServer->on("/", [this]() { handleRoot(); });
  webServer->on("/config", HTTP_POST, [this]() { handleConfig(); });
  webServer->onNotFound([this]() { handleNotFound(); });
  
  webServer->begin();
  
  apModeStarted = true;
  ESP_LOGI(getLogTag(), "AP mode started, IP: %s", WiFi.softAPIP().toString().c_str());
  ESP_LOGI(getLogTag(), "Connect to WiFi network: %s", deviceAPName.c_str());
  ESP_LOGI(getLogTag(), "Then open browser to: http://192.168.4.1");
}

void ProvisionState::stopAPMode()
{
  ESP_LOGI(getLogTag(), "Stopping AP mode");
  
  if (webServer) {
    webServer->stop();
    delete webServer;
    webServer = nullptr;
  }
  
  if (dnsServer) {
    dnsServer->stop();
    delete dnsServer;
    dnsServer = nullptr;
  }
  
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  
  apModeStarted = false;
}

void ProvisionState::handleRoot()
{
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: #f0f0f0;
    }
    .container {
      max-width: 400px;
      margin: 0 auto;
      background: white;
      padding: 30px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 30px;
    }
    input {
      width: 100%;
      padding: 12px;
      margin: 8px 0;
      box-sizing: border-box;
      border: 2px solid #ddd;
      border-radius: 5px;
      font-size: 16px;
    }
    input:focus {
      outline: none;
      border-color: #4CAF50;
    }
    button {
      width: 100%;
      background-color: #4CAF50;
      color: white;
      padding: 14px;
      margin: 20px 0;
      border: none;
      border-radius: 5px;
      font-size: 18px;
      cursor: pointer;
    }
    button:hover {
      background-color: #45a049;
    }
    .device-info {
      text-align: center;
      color: #666;
      margin-bottom: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Timer Device Setup</h1>
    <div class="device-info">)" + deviceAPName + R"(</div>
    <form action="/config" method="POST">
      <input type="text" name="ssid" placeholder="WiFi Network Name" required>
      <input type="password" name="password" placeholder="WiFi Password" required>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)";
  
  webServer->send(200, "text/html", html);
}

void ProvisionState::handleConfig()
{
  if (webServer->hasArg("ssid") && webServer->hasArg("password")) {
    ssid = webServer->arg("ssid");
    password = webServer->arg("password");
    
    ESP_LOGI(getLogTag(), "Received credentials for SSID: %s", ssid.c_str());
    
    String response = R"(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: #f0f0f0;
      text-align: center;
    }
    .message {
      max-width: 400px;
      margin: 50px auto;
      background: white;
      padding: 30px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h2 { color: #4CAF50; }
    p { color: #666; }
  </style>
</head>
<body>
  <div class="message">
    <h2>Configuration Received!</h2>
    <p>Attempting to connect to WiFi network...</p>
    <p>The device will restart once connected.</p>
  </div>
</body>
</html>
)";
    
    webServer->send(200, "text/html", response);
    credentialsReceived = true;
  } else {
    webServer->send(400, "text/plain", "Missing parameters");
  }
}

void ProvisionState::handleNotFound()
{
  // Redirect all requests to the root page for captive portal
  webServer->sendHeader("Location", "/", true);
  webServer->send(302, "text/plain", "");
}

bool ProvisionState::connectToWiFi(const String& ssid, const String& password)
{
  ESP_LOGI(getLogTag(), "Connecting to WiFi: %s", ssid.c_str());
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait for connection (max 30 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    ESP_LOGV(getLogTag(), "Connecting... %d", attempts);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    ESP_LOGI(getLogTag(), "Connected! IP: %s", WiFi.localIP().toString().c_str());
    return true;
  } else {
    ESP_LOGE(getLogTag(), "Connection failed. Status: %d", WiFi.status());
    return false;
  }
}

void ProvisionState::saveCredentials(const String& ssid, const String& password)
{
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("configured", true);
  preferences.end();
  
  ESP_LOGI(getLogTag(), "Credentials saved to NVS");
}

bool ProvisionState::loadCredentials(String& ssid, String& password)
{
  preferences.begin("wifi", true);
  bool configured = preferences.getBool("configured", false);
  
  if (configured) {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
  }
  
  preferences.end();
  return configured;
}

void ProvisionState::showProvisioningUI()
{
  // Update screen to show provisioning instructions
  screenManager.showProvisionScreen(deviceAPName);
  showingCredentialEntry = true;
}

void ProvisionState::updateProvisioningUI()
{
  // Update UI to show connection failed message
  screenManager.showProvisionError("Connection failed. Please try again.");
}

String ProvisionState::generateAPName()
{
  // Use static name "TheTimer"
  return String("TheTimer");
}