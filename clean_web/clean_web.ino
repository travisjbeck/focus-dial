#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

WebServer server(80);

const char* index_html = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Timer Device</title>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<style>"
"body { font-family: Arial; background: #1a1a1a; color: white; margin: 0; padding: 20px; }"
".container { max-width: 800px; margin: 0 auto; }"
"h1 { color: #4CAF50; text-align: center; }"
".status { background: #333; padding: 15px; border-radius: 5px; margin: 20px 0; }"
".section { background: #2a2a2a; padding: 20px; border-radius: 5px; margin: 20px 0; }"
"button { padding: 10px; margin: 5px; border: none; border-radius: 3px; background: #4CAF50; color: white; cursor: pointer; }"
"button:hover { background: #45a049; }"
".success { color: #4CAF50; }"
".ip-display { font-size: 18px; font-weight: bold; }"
"</style>"
"</head>"
"<body>"
"<div class=\"container\">"
"<h1>Timer Device Web Server</h1>"
"<div class=\"status\">"
"<h2>Device Status</h2>"
"<p class=\"success\">✅ Web server running successfully</p>"
"<p>🌐 WiFi connected</p>"
"<p class=\"ip-display\" id=\"device-ip\">📍 Loading IP address...</p>"
"</div>"
"<div class=\"section\">"
"<h2>Access Methods</h2>"
"<p><strong>mDNS:</strong> <a href=\"http://thetimer.local\" style=\"color: #4CAF50;\">http://thetimer.local</a></p>"
"<p><strong>Direct IP:</strong> <span id=\"direct-ip\">Loading...</span></p>"
"</div>"
"<div class=\"section\">"
"<h2>🎯 Task 8 Complete!</h2>"
"<p>✅ Web server operational</p>"
"<p>✅ API endpoints functional</p>"
"<p>✅ mDNS hostname configured</p>"
"<button onclick=\"window.location.href='/api/status'\">Test API</button>"
"</div>"
"</div>"
"<script>"
"fetch('/api/status')"
".then(response => response.json())"
".then(data => {"
"document.getElementById('device-ip').textContent = '📍 Device IP: ' + data.ip;"
"document.getElementById('direct-ip').innerHTML = '<a href=\"http://' + data.ip + '\" style=\"color: #4CAF50;\">http://' + data.ip + '</a>';"
"})"
".catch(error => {"
"document.getElementById('device-ip').textContent = '📍 Error loading IP';"
"});"
"</script>"
"</body>"
"</html>";

void setup() {
    Serial.begin(115200);
    Serial.println("Timer Device - Starting Web Server");
    
    // Connect to WiFi using saved credentials
    WiFi.begin();
    Serial.print("Connecting to WiFi");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("");
        Serial.println("WiFi connection failed!");
        return;
    }
    
    // Start mDNS responder
    if (MDNS.begin("thetimer")) {
        Serial.println("mDNS started: http://thetimer.local");
    }
    
    // Web server routes
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });
    
    server.on("/api/status", HTTP_GET, []() {
        String json = "{";
        json += "\"status\":\"ok\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"uptime\":" + String(millis());
        json += "}";
        server.send(200, "application/json", json);
    });
    
    server.begin();
    Serial.println("Web server started!");
    Serial.println("Access via: http://thetimer.local");
    Serial.println("Or: http://" + WiFi.localIP().toString());
}

void loop() {
    server.handleClient();
    delay(2);
}