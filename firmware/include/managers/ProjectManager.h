#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ProjectData.h"

class ProjectManager
{
public:
  ProjectManager();

  // Call in setup() to load data from NVS
  bool begin();

  // Accessors
  const ProjectList &getProjects() const;
  int getLastProjectIndex() const;

  // Modifiers (handle NVS saving internally)
  bool addProject(const JsonObject &projectData);
  bool updateProject(int index, const Project &project);
  bool deleteProject(int index);
  bool deleteProjectById(const String &deviceProjectId);
  void setLastProjectIndex(int index);

private:
  Preferences _preferences;
  ProjectList _projects;
  int _lastProjectIndex;

  // NVS interaction helpers
  bool _loadProjectsFromNVS();
  bool _saveProjectsToNVS();
  bool _loadLastIndexFromNVS();
  bool _saveLastIndexToNVS();

  // JSON serialization/deserialization
  bool _serializeProjects(JsonDocument &doc);
  bool _deserializeProjects(JsonDocument &doc);

  // Unique ID generation
  String _generateNextDeviceId();
};

#endif // PROJECT_MANAGER_H