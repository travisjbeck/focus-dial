#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <vector>

// Maximum number of projects
const int MAX_PROJECTS = 10;

// Project structure
struct Project {
    String name;
    String color;  // Hex color string (e.g., "#FF0000")
};

class ProjectManager {
public:
    static ProjectManager& getInstance() {
        static ProjectManager instance;
        return instance;
    }
    
    void init();
    void selectProject(int index);
    int getSelectedProjectIndex() const { return selectedProjectIndex; }
    const char* getSelectedProjectName() const;
    uint32_t getSelectedProjectColor() const;
    
    // Save/load from NVS
    void saveToNVS();
    void loadFromNVS();
    
    // Project management
    bool addProject(const String& name, const String& color);
    bool updateProject(int index, const String& name, const String& color);
    bool deleteProject(int index);
    
    // Web API access
    int getProjectCount() const { return projects.size(); }
    const Project* getProject(int index) const { 
        if (index >= 0 && index < projects.size()) return &projects[index]; 
        return nullptr;
    }
    const std::vector<Project>& getAllProjects() const { return projects; }
    
    // Webhook and API key management
    String getWebhookURL() const { return webhookURL; }
    void setWebhookURL(const String& url);
    String getAPIKey() const { return apiKey; }
    void setAPIKey(const String& key);
    
    // Convert hex string to RGB color
    uint32_t hexToRGB(const String& hex) const;
    
    // Force reload of projects (useful after web updates)
    void reloadProjects() { loadProjectsFromNVS(); }
    
private:
    ProjectManager() : selectedProjectIndex(0) {}
    
    // Save/load projects and settings
    void saveProjectsToNVS();
    void loadProjectsFromNVS();
    void saveSettingsToNVS();
    void loadSettingsFromNVS();
    
    int selectedProjectIndex;
    std::vector<Project> projects;
    String webhookURL;
    String apiKey;
    Preferences preferences;
};