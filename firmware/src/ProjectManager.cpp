#include "ProjectManager.h"

// Mock project data
const Project ProjectManager::projects[] = {
    {"No Project", 0x888888},  // Gray
    {"Work", 0x0080FF},        // Blue
    {"Personal", 0x00FF80},    // Green
    {"Exercise", 0xFF8000},    // Orange
    {"Study", 0x8000FF},       // Purple
    {"Hobby", 0xFFFF00}        // Yellow
};

const int ProjectManager::PROJECT_COUNT = 6;

void ProjectManager::init() {
    // Load last selected project from NVS
    loadFromNVS();
}

void ProjectManager::selectProject(int index) {
    if (index >= 0 && index < PROJECT_COUNT) {
        selectedProjectIndex = index;
        saveToNVS();
    }
}

const char* ProjectManager::getSelectedProjectName() const {
    if (selectedProjectIndex >= 0 && selectedProjectIndex < PROJECT_COUNT) {
        return projects[selectedProjectIndex].name;
    }
    return "No Project";
}

uint32_t ProjectManager::getSelectedProjectColor() const {
    if (selectedProjectIndex >= 0 && selectedProjectIndex < PROJECT_COUNT) {
        return projects[selectedProjectIndex].color;
    }
    return 0x888888; // Default gray
}

void ProjectManager::saveToNVS() {
    preferences.begin("timer", false);
    preferences.putInt("projectIdx", selectedProjectIndex);
    preferences.end();
}

void ProjectManager::loadFromNVS() {
    preferences.begin("timer", true);
    selectedProjectIndex = preferences.getInt("projectIdx", 0); // Default to "No Project"
    preferences.end();
    
    // Validate loaded index
    if (selectedProjectIndex < 0 || selectedProjectIndex >= PROJECT_COUNT) {
        selectedProjectIndex = 0;
    }
}