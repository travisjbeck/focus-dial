#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting minimal web server with LittleFS...");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS mounted successfully");
    
    // Connect to WiFi (use existing credentials)
    WiFi.begin();
    Serial.println("Connecting to WiFi...");
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi connection failed!");
        return;
    }
    
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start mDNS
    if (MDNS.begin("thetimer")) {
        Serial.println("mDNS started: http://thetimer.local");
    }
    
    // Serve files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // API endpoint for testing
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"status\":\"ok\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        request->send(200, "application/json", json);
    });
    
    server.begin();
    Serial.println("Web server started!");
    Serial.println("Visit: http://" + WiFi.localIP().toString());
    Serial.println("Or: http://thetimer.local");
}

void loop() {
    delay(1000);
}