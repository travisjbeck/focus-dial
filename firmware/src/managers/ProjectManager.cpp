#include "managers/ProjectManager.h"
#include <esp_system.h>  // For esp_efuse_mac_get_default
#include <Preferences.h> // Ensure Preferences is included

// --- Define the NVS keys declared as extern in ProjectData.h ---
const char *NVS_PROJECTS_KEY = "projects";
const char *NVS_LAST_PROJECT_KEY = "lastProjIdx";
const char *NVS_PROJECT_ID_COUNTER_KEY = "projIdCntr"; // Key for the ID counter

// --- Helper function to get Chip ID as String ---
String getChipId()
{
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  char chipIdStr[13]; // 6 bytes MAC * 2 chars/byte + 1 null terminator
  snprintf(chipIdStr, sizeof(chipIdStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(chipIdStr);
}

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

bool ProjectManager::addProject(const JsonObject &projectData)
{
  if (_projects.size() >= MAX_PROJECTS)
  {
    Serial.println("ProjectManager: Max projects reached.");
    return false;
  }

  // Validate incoming data
  if (!projectData.containsKey("name") || !projectData["name"].is<const char *>() ||
      !projectData.containsKey("color") || !projectData["color"].is<const char *>())
  {
    Serial.println("ProjectManager: Invalid project data (missing name/color).");
    return false;
  }

  Project newProject;
  newProject.name = projectData["name"].as<String>();
  newProject.color = projectData["color"].as<String>();

  // Basic validation
  if (newProject.name.isEmpty() || !newProject.color.startsWith("#") || newProject.color.length() != 7)
  {
    Serial.println("ProjectManager: Invalid project format (name empty or invalid color).");
    return false;
  }

  // --- Generate and assign unique ID ---
  newProject.device_project_id = _generateNextDeviceId();
  if (newProject.device_project_id.isEmpty())
  {
    Serial.println("ProjectManager: Failed to generate device project ID.");
    return false; // Stop if ID generation fails
  }
  Serial.printf("Generated Device Project ID: %s\n", newProject.device_project_id.c_str());
  // ----------------------------------

  _projects.push_back(newProject);
  return _saveProjectsToNVS();
}

bool ProjectManager::updateProject(int index, const Project &updatedData)
{
  if (index < 0 || index >= _projects.size())
  {
    Serial.println("ProjectManager: Invalid index for update.");
    return false;
  }

  // Basic validation on incoming data
  if (updatedData.name.isEmpty() || !updatedData.color.startsWith("#") || updatedData.color.length() != 7)
  {
    Serial.println("ProjectManager: Invalid project data for update.");
    return false;
  }

  // Preserve the existing device_project_id
  String existingId = _projects[index].device_project_id;

  // Assign new name and color
  _projects[index].name = updatedData.name;
  _projects[index].color = updatedData.color;

  // Ensure the ID is kept (or assigned if it was somehow missing)
  if (existingId.isEmpty())
  {
    Serial.printf("ProjectManager: Warning - Project at index %d was missing ID. Generating new one.\n", index);
    _projects[index].device_project_id = _generateNextDeviceId();
    if (_projects[index].device_project_id.isEmpty())
    {
      Serial.println("ProjectManager: Failed to generate missing ID during update.");
      return false; // Fail update if ID generation fails
    }
  }
  else
  {
    _projects[index].device_project_id = existingId;
  }

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

bool ProjectManager::deleteProjectById(const String &deviceProjectId)
{
  if (deviceProjectId.isEmpty())
  {
    Serial.println("ProjectManager::deleteProjectById: Empty device_project_id provided.");
    return false;
  }

  int indexToDelete = -1;

  // Find the project with the matching ID
  for (size_t i = 0; i < _projects.size(); i++)
  {
    if (_projects[i].device_project_id == deviceProjectId)
    {
      indexToDelete = i;
      break;
    }
  }

  if (indexToDelete == -1)
  {
    Serial.printf("ProjectManager::deleteProjectById: No project found with ID %s\n", deviceProjectId.c_str());
    return false;
  }

  // Call the existing index-based delete method
  return deleteProject(indexToDelete);
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
    JsonObject obj = array.add<JsonObject>();
    obj["name"] = project.name;
    obj["color"] = project.color;
    obj["device_project_id"] = project.device_project_id; // Always serialize
  }
  return true;
}

bool ProjectManager::_deserializeProjects(JsonDocument &doc)
{
  _projects.clear(); // Start with an empty list
  JsonArray array = doc.as<JsonArray>();

  if (_projects.capacity() < array.size())
  {
    _projects.reserve(array.size());
  }

  bool needsSave = false; // Flag to check if we need to re-save NVS

  for (JsonObject obj : array)
  {
    if (_projects.size() >= MAX_PROJECTS)
    {
      Serial.println("ProjectManager: Max projects reached during NVS load.");
      break;
    }
    Project p;
    // Check for mandatory fields first
    if (obj["name"].is<const char *>() && obj["color"].is<const char *>())
    {
      p.name = obj["name"].as<String>();
      p.color = obj["color"].as<String>();

      // Deserialize device_project_id if present, generate if missing
      if (obj.containsKey("device_project_id") && obj["device_project_id"].is<const char *>() && !String(obj["device_project_id"].as<const char *>()).isEmpty())
      {
        p.device_project_id = obj["device_project_id"].as<String>();
      }
      else
      {
        // Generate ID if missing or empty from old data
        Serial.printf("ProjectManager: Generating missing ID for loaded project '%s'\n", p.name.c_str());
        p.device_project_id = _generateNextDeviceId();
        if (p.device_project_id.isEmpty())
        {
          Serial.println("ProjectManager: CRITICAL - Failed to generate missing ID during deserialization. Skipping project.");
          continue; // Skip this project if ID generation fails
        }
        needsSave = true; // Mark that we need to save the updated list
      }

      // Basic validation on load
      if (!p.name.isEmpty() && p.color.startsWith("#") && p.color.length() == 7)
      {
        _projects.push_back(p);
      }
      else
      {
        Serial.println("ProjectManager: Skipping invalid project name/color during load.");
      }
    }
    else
    {
      Serial.println("ProjectManager: Skipping malformed project object (missing name/color) during load.");
    }
  }

  // If we generated any missing IDs, save the updated list back to NVS
  if (needsSave)
  {
    Serial.println("ProjectManager: Saving projects back to NVS after generating missing IDs.");
    _saveProjectsToNVS();
  }

  return true;
}

String ProjectManager::_generateNextDeviceId()
{
  if (!_preferences.begin(PROJECT_MANAGER_NVS_NAMESPACE, false))
  {
    Serial.println("ProjectManager: Failed to open NVS for generating ID!");
    return ""; // Return empty string on failure
  }

  // Get the current counter value, default to 0 if not found
  uint32_t counter = _preferences.getUInt(NVS_PROJECT_ID_COUNTER_KEY, 0);

  // Increment the counter for the next ID
  counter++;

  // Save the updated counter back to NVS
  bool saved = _preferences.putUInt(NVS_PROJECT_ID_COUNTER_KEY, counter);

  _preferences.end(); // Close NVS

  if (!saved)
  {
    Serial.println("ProjectManager: Failed to save project ID counter!");
    return ""; // Return empty string on failure
  }

  // Construct the ID string: ChipID-Counter
  String chipId = getChipId();
  if (chipId.isEmpty())
  {
    Serial.println("ProjectManager: Failed to get Chip ID!");
    return "";
  }

  String deviceId = chipId + "-" + String(counter);
  return deviceId;
}