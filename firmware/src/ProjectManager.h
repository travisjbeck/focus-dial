#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Project structure
struct Project {
    const char* name;
    uint32_t color;
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
    
private:
    ProjectManager() : selectedProjectIndex(0) {}
    int selectedProjectIndex;
    Preferences preferences;
    
    // Mock projects - will be replaced with dynamic list later
    static const Project projects[];
    static const int PROJECT_COUNT;
};