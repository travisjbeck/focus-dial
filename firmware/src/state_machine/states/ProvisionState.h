#pragma once

#include "../include/State.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

class ProvisionState : public State
{
public:
  ProvisionState();
  virtual ~ProvisionState();
  
  const char* getStateName() const override { return "ProvisionState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "ProvisionState"; }

private:
  bool apModeStarted;
  WebServer* webServer;
  DNSServer* dnsServer;
  Preferences preferences;
  String deviceAPName;
  
  // WiFi credentials
  String ssid;
  String password;
  bool credentialsReceived;
  
  // UI state
  bool showingCredentialEntry;
  
  // Methods
  void startAPMode();
  void stopAPMode();
  void handleRoot();
  void handleConfig();
  void handleNotFound();
  bool connectToWiFi(const String& ssid, const String& password);
  void saveCredentials(const String& ssid, const String& password);
  bool loadCredentials(String& ssid, String& password);
  void showProvisioningUI();
  void updateProvisioningUI();
  String generateAPName();
};