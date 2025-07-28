#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

WebServer server(80);

const char* html_content = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Timer Device - Project Management</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', sans-serif;
            background: #000;
            color: #fff;
            line-height: 1.6;
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        .header {
            text-align: center;
            padding: 40px 0;
            border-bottom: 1px solid #333;
            margin-bottom: 40px;
        }
        .header h1 {
            font-size: 2.5rem;
            font-weight: 600;
            margin-bottom: 0.5rem;
        }
        .status {
            background: #111;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
            border: 1px solid #333;
        }
        .section {
            background: #111;
            padding: 30px;
            border-radius: 8px;
            margin-bottom: 30px;
            border: 1px solid #333;
        }
        .section h2 {
            font-size: 1.5rem;
            margin-bottom: 20px;
            color: #fff;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #ccc;
        }
        input, select, button {
            width: 100%;
            padding: 12px;
            border: 1px solid #444;
            border-radius: 6px;
            background: #222;
            color: #fff;
            font-size: 14px;
        }
        button {
            background: #0070f3;
            color: white;
            cursor: pointer;
            font-weight: 500;
            transition: background 0.2s;
        }
        button:hover {
            background: #0051cc;
        }
        .project-list {
            list-style: none;
            margin-top: 20px;
        }
        .project-item {
            background: #222;
            padding: 15px;
            margin-bottom: 10px;
            border-radius: 6px;
            border: 1px solid #333;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .success { color: #22c55e; }
        .error { color: #ef4444; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Timer Device</h1>
            <p>Project Management Interface</p>
        </div>
        
        <div class="status">
            <h2>Device Status</h2>
            <p>✅ Web server running successfully</p>
            <p>🌐 Connected to WiFi</p>
            <p id="device-ip">📍 Device IP: Loading...</p>
        </div>

        <div class="section">
            <h2>Add New Project</h2>
            <form id="add-project-form">
                <div class="form-group">
                    <label for="project-name">Project Name</label>
                    <input type="text" id="project-name" required>
                </div>
                <div class="form-group">
                    <label for="project-color">LED Color (hex)</label>
                    <input type="text" id="project-color" placeholder="#FF0000" required>
                </div>
                <button type="submit">Add Project</button>
            </form>
        </div>

        <div class="section">
            <h2>Current Projects</h2>
            <ul class="project-list" id="project-list">
                <li class="project-item">Loading projects...</li>
            </ul>
        </div>
    </div>

    <script>
        // Load device IP
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('device-ip').textContent = `📍 Device IP: ${data.ip}`;
            })
            .catch(error => {
                document.getElementById('device-ip').textContent = '📍 Device IP: Error loading';
            });

        // Load projects (placeholder)
        document.getElementById('project-list').innerHTML = 
            '<li class="project-item">📝 Project functionality ready for implementation</li>';

        // Handle form submission
        document.getElementById('add-project-form').addEventListener('submit', function(e) {
            e.preventDefault();
            const name = document.getElementById('project-name').value;
            const color = document.getElementById('project-color').value;
            alert(`Project "${name}" with color ${color} would be added to device.`);
        });
    </script>
</body>
</html>
)";

void setup() {
    Serial.begin(115200);
    Serial.println("Starting embedded web server...");
    
    // Connect to WiFi
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
    
    // Serve embedded HTML
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", html_content);
    });
    
    // API endpoint for status
    server.on("/api/status", []() {
        String json = "{\"status\":\"ok\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        server.send(200, "application/json", json);
    });
    
    server.begin();
    Serial.println("Web server started!");
    Serial.println("Visit: http://" + WiFi.localIP().toString());
    Serial.println("Or: http://thetimer.local");
}

void loop() {
    server.handleClient();
    delay(10);
}