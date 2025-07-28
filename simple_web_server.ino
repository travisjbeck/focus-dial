#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

WebServer server(80);

// Simple HTML page embedded in firmware
const char* index_html = R"(<!DOCTYPE html>
<html>
<head>
    <title>Timer Device</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial; background: #1a1a1a; color: white; margin: 0; padding: 20px; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #4CAF50; text-align: center; }
        .status { background: #333; padding: 15px; border-radius: 5px; margin: 20px 0; }
        .section { background: #2a2a2a; padding: 20px; border-radius: 5px; margin: 20px 0; }
        input, button { padding: 10px; margin: 5px; border: none; border-radius: 3px; }
        button { background: #4CAF50; color: white; cursor: pointer; }
        button:hover { background: #45a049; }
        .success { color: #4CAF50; }
        .ip-display { font-size: 18px; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔥 Timer Device Web Server</h1>
        
        <div class="status">
            <h2>Device Status</h2>
            <p class="success">✅ Web server running successfully</p>
            <p>🌐 WiFi connected</p>
            <p class="ip-display" id="device-ip">📍 Loading IP address...</p>
        </div>

        <div class="section">
            <h2>Access Methods</h2>
            <p>🔗 <strong>mDNS:</strong> <a href="http://thetimer.local" style="color: #4CAF50;">http://thetimer.local</a></p>
            <p>🔗 <strong>Direct IP:</strong> <span id="direct-ip">Loading...</span></p>
        </div>

        <div class="section">
            <h2>API Endpoints</h2>
            <p>📊 <a href="/api/status" style="color: #4CAF50;">/api/status</a> - Device status</p>
            <p>📁 <a href="/api/files" style="color: #4CAF50;">/api/files</a> - File system info</p>
        </div>

        <div class="section">
            <h2>Project Management</h2>
            <p>🚀 Web interface ready for timer project management</p>
            <button onclick="testApi()">Test API Connection</button>
            <div id="api-result"></div>
        </div>
    </div>

    <script>
        // Load device IP and update page
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('device-ip').textContent = '📍 Device IP: ' + data.ip;
                document.getElementById('direct-ip').innerHTML = 
                    '<a href="http://' + data.ip + '" style="color: #4CAF50;">http://' + data.ip + '</a>';
            })
            .catch(error => {
                document.getElementById('device-ip').textContent = '📍 Error loading IP';
            });

        function testApi() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('api-result').innerHTML = 
                        '<p class="success">✅ API working! Status: ' + data.status + '</p>';
                })
                .catch(error => {
                    document.getElementById('api-result').innerHTML = 
                        '<p style="color: red;">❌ API test failed</p>';
                });
        }
    </script>
</body>
</html>)";

void setup() {
    Serial.begin(115200);
    Serial.println("Timer Device - Starting Web Server");
    
    // Initialize LittleFS (optional - files are embedded)
    if (LittleFS.begin()) {
        Serial.println("LittleFS mounted successfully");
    } else {
        Serial.println("LittleFS mount failed, using embedded content");
    }
    
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
        Serial.println("WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println("");
        Serial.println("WiFi connection failed!");
        return;
    }
    
    // Start mDNS responder
    if (MDNS.begin("thetimer")) {
        Serial.println("mDNS responder started");
        Serial.println("Access via: http://thetimer.local");
    } else {
        Serial.println("Error starting mDNS");
    }
    
    // Web server routes
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-cache");
        server.send(200, "text/html", index_html);
    });
    
    server.on("/api/status", HTTP_GET, []() {
        String json = "{";
        json += "\"status\":\"ok\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis());
        json += "}";
        server.send(200, "application/json", json);
    });
    
    server.on("/api/files", HTTP_GET, []() {
        String result = "LittleFS Status:\n";
        if (LittleFS.begin()) {
            result += "✅ Mounted successfully\n";
            result += "Total: " + String(LittleFS.totalBytes()) + " bytes\n";
            result += "Used: " + String(LittleFS.usedBytes()) + " bytes\n";
            result += "Files:\n";
            
            File root = LittleFS.open("/");
            if (root && root.isDirectory()) {
                File file = root.openNextFile();
                while (file) {
                    result += "  📄 " + String(file.name()) + " (" + String(file.size()) + " bytes)\n";
                    file = root.openNextFile();
                }
            }
        } else {
            result += "❌ LittleFS not available\n";
        }
        server.send(200, "text/plain", result);
    });
    
    // Handle 404
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Page not found");
    });
    
    // Start web server
    server.begin();
    Serial.println("HTTP server started on port 80");
    Serial.println("=================================");
    Serial.println("🔥 Timer Device Web Server Ready");
    Serial.println("Access methods:");
    Serial.println("  • http://thetimer.local");
    Serial.println("  • http://" + WiFi.localIP().toString());
    Serial.println("=================================");
}

void loop() {
    server.handleClient();
    MDNS.update();  // Keep mDNS alive
    delay(2);
}