#include "ProjectManager.h"

void ProjectManager::init() {
    // Load projects and settings from NVS
    loadFromNVS();
    
    // If no projects exist, create default ones
    if (projects.empty()) {
        projects.push_back({"No Project", "#888888"});
        projects.push_back({"Work", "#0080FF"});
        projects.push_back({"Personal", "#00FF80"});
        projects.push_back({"Exercise", "#FF8000"});
        projects.push_back({"Study", "#8000FF"});
        projects.push_back({"Hobby", "#FFFF00"});
        saveProjectsToNVS();
    }
}

void ProjectManager::selectProject(int index) {
    if (index >= 0 && index < projects.size()) {
        selectedProjectIndex = index;
        saveToNVS();
    }
}

const char* ProjectManager::getSelectedProjectName() const {
    if (selectedProjectIndex >= 0 && selectedProjectIndex < projects.size()) {
        return projects[selectedProjectIndex].name.c_str();
    }
    return "No Project";
}

uint32_t ProjectManager::getSelectedProjectColor() const {
    if (selectedProjectIndex >= 0 && selectedProjectIndex < projects.size()) {
        return hexToRGB(projects[selectedProjectIndex].color);
    }
    return 0x888888; // Default gray
}

bool ProjectManager::addProject(const String& name, const String& color) {
    if (projects.size() >= MAX_PROJECTS) {
        return false;
    }
    
    projects.push_back({name, color});
    saveProjectsToNVS();
    return true;
}

bool ProjectManager::updateProject(int index, const String& name, const String& color) {
    if (index < 0 || index >= projects.size()) {
        return false;
    }
    
    projects[index].name = name;
    projects[index].color = color;
    saveProjectsToNVS();
    return true;
}

bool ProjectManager::deleteProject(int index) {
    if (index < 0 || index >= projects.size()) {
        return false;
    }
    
    projects.erase(projects.begin() + index);
    
    // Adjust selected index if needed
    if (selectedProjectIndex == index) {
        selectedProjectIndex = 0; // Reset to "No Project"
    } else if (selectedProjectIndex > index) {
        selectedProjectIndex--;
    }
    
    saveProjectsToNVS();
    saveToNVS(); // Save the updated selection index
    return true;
}

void ProjectManager::setWebhookURL(const String& url) {
    webhookURL = url;
    saveSettingsToNVS();
}

void ProjectManager::setAPIKey(const String& key) {
    apiKey = key;
    saveSettingsToNVS();
}

void ProjectManager::saveToNVS() {
    preferences.begin("timer", false);
    preferences.putInt("projectIdx", selectedProjectIndex);
    preferences.end();
}

void ProjectManager::loadFromNVS() {
    // Load selected project index
    preferences.begin("timer", true);
    selectedProjectIndex = preferences.getInt("projectIdx", 0);
    preferences.end();
    
    // Load projects
    loadProjectsFromNVS();
    
    // Load settings (webhook, API key)
    loadSettingsFromNVS();
    
    // Validate selected index
    if (selectedProjectIndex < 0 || selectedProjectIndex >= projects.size()) {
        selectedProjectIndex = 0;
    }
}

void ProjectManager::saveProjectsToNVS() {
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (const auto& project : projects) {
        JsonObject obj = array.createNestedObject();
        obj["name"] = project.name;
        obj["color"] = project.color;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    preferences.begin("timer", false);
    preferences.putString("projects", jsonString);
    preferences.end();
}

void ProjectManager::loadProjectsFromNVS() {
    preferences.begin("timer", true);
    String jsonString = preferences.getString("projects", "");
    preferences.end();
    
    if (jsonString.isEmpty()) {
        return; // No saved projects
    }
    
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.print("Failed to parse projects JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    projects.clear();
    JsonArray array = doc.as<JsonArray>();
    
    for (JsonObject obj : array) {
        if (projects.size() >= MAX_PROJECTS) break;
        
        String name = obj["name"] | "";
        String color = obj["color"] | "#888888";
        
        if (!name.isEmpty()) {
            projects.push_back({name, color});
        }
    }
}

void ProjectManager::saveSettingsToNVS() {
    preferences.begin("timer", false);
    preferences.putString("webhook_url", webhookURL);
    preferences.putString("api_key", apiKey);
    preferences.end();
}

void ProjectManager::loadSettingsFromNVS() {
    preferences.begin("timer", true);
    webhookURL = preferences.getString("webhook_url", "");
    apiKey = preferences.getString("api_key", "");
    preferences.end();
}

uint32_t ProjectManager::hexToRGB(const String& hex) const {
    if (hex.length() != 7 || hex[0] != '#') {
        return 0x888888; // Default gray
    }
    
    // Convert hex string to RGB value
    long rgb = strtol(hex.c_str() + 1, nullptr, 16);
    return (uint32_t)rgb;
}