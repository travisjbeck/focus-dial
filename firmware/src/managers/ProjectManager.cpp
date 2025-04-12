#include "managers/ProjectManager.h"

// --- Define the NVS keys declared as extern in ProjectData.h ---
const char *NVS_PROJECTS_KEY = "projects";
const char *NVS_LAST_PROJECT_KEY = "lastProjIdx";

// Define the NVS namespace used by ProjectManager
const char *PROJECT_MANAGER_NVS_NAMESPACE = "projects";

ProjectManager::ProjectManager() : _lastProjectIndex(-1) // Initialize last index to -1 (invalid)
{
  // Constructor - potentially initialize Preferences object here if needed
}

bool ProjectManager::begin()
{
  // Open NVS namespace
  if (!_preferences.begin(PROJECT_MANAGER_NVS_NAMESPACE, false))
  {
    Serial.println("ProjectManager: Failed to initialize NVS!");
    return false;
  }
  Serial.println("ProjectManager: NVS initialized.");

  // Load data
  bool loadProjectsOk = _loadProjectsFromNVS();
  bool loadIndexOk = _loadLastIndexFromNVS();

  _preferences.end(); // Close NVS until needed again
  return loadProjectsOk && loadIndexOk;
}

const ProjectList &ProjectManager::getProjects() const
{
  return _projects;
}

int ProjectManager::getLastProjectIndex() const
{
  return _lastProjectIndex;
}

// --- Modifiers ---

bool ProjectManager::addProject(const Project &project)
{
  if (_projects.size() >= MAX_PROJECTS)
  {
    Serial.println("ProjectManager: Max projects reached.");
    return false;
  }
  // Basic validation (e.g., non-empty name? valid color format?)
  if (project.name.isEmpty() || !project.color.startsWith("#") || project.color.length() != 7)
  {
    Serial.println("ProjectManager: Invalid project data.");
    return false;
  }

  _projects.push_back(project);
  return _saveProjectsToNVS();
}

bool ProjectManager::updateProject(int index, const Project &project)
{
  if (index < 0 || index >= _projects.size())
  {
    Serial.println("ProjectManager: Invalid index for update.");
    return false;
  }
  // Basic validation
  if (project.name.isEmpty() || !project.color.startsWith("#") || project.color.length() != 7)
  {
    Serial.println("ProjectManager: Invalid project data.");
    return false;
  }

  _projects[index] = project;
  return _saveProjectsToNVS();
}

bool ProjectManager::deleteProject(int index)
{
  if (index < 0 || index >= _projects.size())
  {
    Serial.println("ProjectManager::deleteProject: Invalid index.");
    return false;
  }
  Serial.printf("ProjectManager::deleteProject: Deleting index %d\n", index);
  _projects.erase(_projects.begin() + index);

  // Adjust last selected index if it was the deleted item or after it
  if (_lastProjectIndex == index)
  {
    Serial.println("ProjectManager::deleteProject: Resetting lastProjectIndex.");
    setLastProjectIndex(-1); // Reset if deleted item was last selected
  }
  else if (_lastProjectIndex > index)
  {
    Serial.printf("ProjectManager::deleteProject: Decrementing lastProjectIndex from %d\n", _lastProjectIndex);
    setLastProjectIndex(_lastProjectIndex - 1); // Decrement if after deleted item
  }

  Serial.println("ProjectManager::deleteProject: Calling _saveProjectsToNVS()...");
  bool saveOk = _saveProjectsToNVS();
  Serial.printf("ProjectManager::deleteProject: _saveProjectsToNVS() returned %s\n", saveOk ? "true" : "false");
  return saveOk;
}

void ProjectManager::setLastProjectIndex(int index)
{
  _lastProjectIndex = index;
  _saveLastIndexToNVS(); // Save immediately
}

// --- Private NVS Interaction Helpers ---

bool ProjectManager::_loadProjectsFromNVS()
{
  // Note: Preferences opened in begin()
  String jsonString = _preferences.getString(NVS_PROJECTS_KEY, "");
  if (jsonString.isEmpty())
  {
    Serial.println("ProjectManager: No projects found in NVS.");
    _projects.clear(); // Ensure list is empty
    return true;       // Not an error if it's just empty
  }

  // Allocate JsonDocument - size needs estimation!
  // TODO: Calculate a reasonable size based on MAX_PROJECTS
  // DynamicJsonDocument doc(2048); // Example size - NEEDS TUNING!
  JsonDocument doc; // Use stack-based JsonDocument for v7
  // The size is determined by the input string, but filter for safety
  JsonDocument filter;
  filter["name"] = true;
  filter["color"] = true;
  JsonArray filterArray = filter.to<JsonArray>();
  filterArray.add(true); // Allow array of objects

  DeserializationError error = deserializeJson(doc, jsonString, DeserializationOption::Filter(filter));

  if (error)
  {
    Serial.print("ProjectManager: deserializeJson() failed: ");
    Serial.println(error.c_str());
    _projects.clear();
    return false;
  }

  if (!doc.is<JsonArray>())
  {
    Serial.println("ProjectManager: NVS data is not a JSON array.");
    _projects.clear();
    return false;
  }

  if (!_deserializeProjects(doc))
  {
    _projects.clear();
    return false;
  }

  Serial.printf("ProjectManager: Loaded %d projects from NVS.\n", _projects.size());
  return true;
}

bool ProjectManager::_saveProjectsToNVS()
{
  // DynamicJsonDocument doc(2048); // Example size - NEEDS TUNING!
  // Calculate required size dynamically based on current projects
  size_t requiredSize = JSON_ARRAY_SIZE(_projects.size());
  for (const auto &p : _projects)
  {
    requiredSize += JSON_OBJECT_SIZE(2) + p.name.length() + p.color.length();
  }
  requiredSize += 256; // Add some buffer

  JsonDocument doc;
  if (!_serializeProjects(doc))
  { // Pass by reference
    return false;
  }

  String jsonString;
  serializeJson(doc, jsonString);

  if (!_preferences.begin(PROJECT_MANAGER_NVS_NAMESPACE, false))
  {
    Serial.println("ProjectManager: Failed to open NVS for saving projects!");
    return false;
  }
  bool success = _preferences.putString(NVS_PROJECTS_KEY, jsonString);
  _preferences.end();

  if (success)
  {
    Serial.printf("ProjectManager: Saved %d projects to NVS.\n", _projects.size());
  }
  else
  {
    Serial.println("ProjectManager: Failed to save projects to NVS.");
  }
  return success;
}

bool ProjectManager::_loadLastIndexFromNVS()
{
  // Note: Preferences opened in begin()
  _lastProjectIndex = _preferences.getInt(NVS_LAST_PROJECT_KEY, -1); // Default to -1 if not found
  Serial.printf("ProjectManager: Loaded last project index: %d\n", _lastProjectIndex);
  return true; // Getting default is not an error
}

bool ProjectManager::_saveLastIndexToNVS()
{
  if (!_preferences.begin(PROJECT_MANAGER_NVS_NAMESPACE, false))
  {
    Serial.println("ProjectManager: Failed to open NVS for saving index!");
    return false;
  }
  bool success = _preferences.putInt(NVS_LAST_PROJECT_KEY, _lastProjectIndex);
  _preferences.end();

  if (success)
  {
    Serial.printf("ProjectManager: Saved last project index: %d\n", _lastProjectIndex);
  }
  else
  {
    Serial.println("ProjectManager: Failed to save last index to NVS.");
  }
  return success;
}

// --- Private JSON Helpers ---

bool ProjectManager::_serializeProjects(JsonDocument &doc)
{ // Accept by reference
  JsonArray array = doc.to<JsonArray>();
  for (const auto &project : _projects)
  {
    // JsonObject obj = array.createNestedObject(); // v6
    JsonObject obj = array.add<JsonObject>(); // v7
    obj["name"] = project.name;
    obj["color"] = project.color;
  }
  return true;
}

bool ProjectManager::_deserializeProjects(JsonDocument &doc)
{
  _projects.clear(); // Start with an empty list
  JsonArray array = doc.as<JsonArray>();

  // Reserve space for efficiency if possible
  if (_projects.capacity() < array.size())
  {
    _projects.reserve(array.size());
  }

  for (JsonObject obj : array)
  {
    if (_projects.size() >= MAX_PROJECTS)
    {
      Serial.println("ProjectManager: Max projects reached during NVS load.");
      break; // Stop loading if limit reached
    }
    Project p;
    // if (obj.containsKey("name") && obj["name"].is<const char*>() &&
    //     obj.containsKey("color") && obj["color"].is<const char*>()) // v6
    if (obj["name"].is<const char *>() && obj["color"].is<const char *>()) // v7 check
    {
      p.name = obj["name"].as<String>();
      p.color = obj["color"].as<String>();

      // Basic validation on load
      if (!p.name.isEmpty() && p.color.startsWith("#") && p.color.length() == 7)
      {
        _projects.push_back(p);
      }
      else
      {
        Serial.println("ProjectManager: Skipping invalid project data during load.");
      }
    }
    else
    {
      Serial.println("ProjectManager: Skipping malformed project object during load.");
    }
  }
  return true;
}