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
  // Scan for available WiFi networks
  int n = WiFi.scanNetworks();
  
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>TheTimer - Provisioning</title>
  <style>
    :root {
      --theme-color: #c2e189;    
      --font-color: #fff;
      --card-background: #171717;
      --black: #080808;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      margin: 0;
      padding: 0;
      background-color: var(--black);
      color: var(--font-color);
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
    }
    .card {
      background: var(--card-background);
      padding: 30px;
      border-radius: 10px;
      margin-bottom: 20px;
    }
    h1 {
      text-align: center;
      margin: 20px 0;
      font-size: 2em;
    }
    .project-info {
      margin: 20px 0;
      line-height: 1.6;
      color: #ccc;
    }
    select, input[type="password"] {
      width: 100%;
      padding: 12px 16px;
      margin: 8px 0;
      box-sizing: border-box;
      border: 1px solid #333;
      border-radius: 8px;
      font-size: 16px;
      background-color: #0a0a0a;
      color: var(--font-color);
      appearance: none;
      transition: border-color 0.2s;
    }
    select {
      background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='white' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e");
      background-repeat: no-repeat;
      background-position: right 12px center;
      background-size: 20px;
      padding-right: 40px;
    }
    select:focus, input:focus {
      outline: none;
      border-color: var(--theme-color);
    }
    button {
      width: 100%;
      background-color: var(--theme-color);
      color: #000;
      padding: 14px;
      margin: 20px 0;
      border: none;
      border-radius: 8px;
      font-size: 18px;
      font-weight: 600;
      cursor: pointer;
      transition: opacity 0.2s;
    }
    button:hover {
      opacity: 0.9;
    }
    .footer {
      text-align: center;
      margin-top: 30px;
      color: #666;
      font-size: 0.9em;
    }
    label {
      display: block;
      margin: 15px 0 5px;
      color: #ccc;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>TheTimer Setup</h1>
    
    <div class="card">
      <div class="project-info">
        Select a WiFi network to save and allow TheTimer to trigger automations.
      </div>
      
      <form action="/config" method="POST">
        <label>Select WiFi Network</label>
        <select name="ssid" required>
          <option value="">Select Network</option>)";
  
  // Add available networks to dropdown
  for (int i = 0; i < n; i++) {
    html += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  
  html += R"(
        </select>
        
        <label>WiFi Password</label>
        <input type="password" name="password" placeholder="Enter your WiFi password" required>
        
        <button type="submit">Connect</button>
      </form>
    </div>
    
    <div class="footer">
      TheTimer - Made by Magic Sauce
    </div>
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
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>TheTimer - Configuration Received</title>
  <style>
    :root {
      --theme-color: #c2e189;    
      --font-color: #fff;
      --card-background: #171717;
      --black: #080808;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      margin: 0;
      padding: 0;
      background-color: var(--black);
      color: var(--font-color);
      text-align: center;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
    }
    .message {
      background: var(--card-background);
      padding: 40px;
      border-radius: 10px;
      margin-top: 50px;
    }
    h2 { 
      color: var(--theme-color);
      margin-bottom: 20px;
    }
    p { 
      color: #ccc;
      line-height: 1.6;
      margin: 10px 0;
    }
    .success-icon {
      font-size: 48px;
      color: var(--theme-color);
      margin-bottom: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="message">
      <div class="success-icon">✓</div>
      <h2>Configuration Received!</h2>
      <p>Provision Complete. TheTimer will now start and status led will turn to blue.</p>
      <p>Attempting to connect to WiFi network...</p>
      <p>The device will restart once connected.</p>
    </div>
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
  // For captive portal, redirect all requests to the root page
  webServer->sendHeader("Location", "http://192.168.4.1/", true);
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

