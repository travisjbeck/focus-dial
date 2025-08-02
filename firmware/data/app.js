// TheTimer Project Management app.js

// Color preview timeout for debouncing
let colorPreviewTimeout = null;

document.addEventListener('DOMContentLoaded', () => {
  console.log('DOM fully loaded');
  fetchAndRenderProjects();
  fetchWebhookUrl();
  fetchApiKey();
  fetchAlarmSettings();

  const form = document.getElementById('add-project-form');
  if (form) {
    form.addEventListener('submit', handleAddProjectSubmit);
  }

  const webhookForm = document.getElementById('webhook-form');
  if (webhookForm) {
    webhookForm.addEventListener('submit', handleWebhookSubmit);
  }

  const alarmForm = document.getElementById('alarm-form');
  if (alarmForm) {
    alarmForm.addEventListener('submit', handleAlarmSubmit);
  }

  const previewButton = document.getElementById('preview-sound');
  if (previewButton) {
    previewButton.addEventListener('click', handlePreviewSound);
  }

  const stopButton = document.getElementById('stop-sound');
  if (stopButton) {
    stopButton.addEventListener('click', handleStopSound);
  }

  // Initialize color picker with default color
  window.selectedColorValue = '#3b82f6';
  window.currentHue = 220;
  window.currentSaturation = 100;
  window.currentLightness = 62;
});

// Function to send color update via HTTP, with debounce
async function sendColorUpdate(colorHex) {
  // Clear any pending timeout
  if (colorPreviewTimeout) {
    clearTimeout(colorPreviewTimeout);
  }

  // Debounce the color updates to prevent flooding the device
  colorPreviewTimeout = setTimeout(async () => {
    try {
      const response = await fetch('/api/color/preview', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `color=${encodeURIComponent(colorHex)}`
      });
      
      if (response.ok) {
        console.log('Sent color preview:', colorHex);
      } else {
        console.error('Error sending color preview:', response.status);
      }
    } catch (error) {
      console.error('Error sending color preview:', error);
    }
  }, 50); // 50ms debounce
}

// Function to send reset command
async function sendResetColorUpdate() {
  // Clear any pending color updates
  if (colorPreviewTimeout) {
    clearTimeout(colorPreviewTimeout);
  }

  try {
    const response = await fetch('/api/color/reset', {
      method: 'POST'
    });
    
    if (response.ok) {
      console.log('Sent color reset');
    } else {
      console.error('Error sending color reset:', response.status);
    }
  } catch (error) {
    console.error('Error sending color reset:', error);
  }
}

const apiBaseUrl = '/api/projects';
const projectListDiv = document.getElementById('project-list');
const messageArea = document.getElementById('message-area');

// --- Webhook URL & API Key Configuration ---
async function fetchWebhookUrl() {
  try {
    const response = await fetch('/api/webhook');
    if (!response.ok) {
      // Silently fail if the endpoint doesn't exist yet - don't show error to user
      console.log('Webhook endpoint not available yet');
      return;
    }
    const data = await response.json();

    const webhookInput = document.getElementById('webhook-url');
    if (webhookInput && data.url) {
      // Clean up the URL if needed before displaying
      let displayUrl = data.url;

      // Check for double protocol issue (e.g., http://HTTPS://...)
      const protocolMatch = displayUrl.match(/^(https?:\/\/)(https?:\/\/)/i);
      if (protocolMatch) {
        // Remove the first protocol prefix if we have a duplicate
        displayUrl = displayUrl.substring(protocolMatch[1].length);
        console.log('Fixed double protocol in URL:', displayUrl);
      }

      webhookInput.value = displayUrl;
    }
  } catch (error) {
    // Just log to console, don't show error message to the user
    console.error('Error fetching webhook URL:', error);
  }
}

// New function to fetch API Key
async function fetchApiKey() {
  try {
    // Assuming a new GET endpoint exists
    const response = await fetch('/api/apikey');
    if (!response.ok) {
      console.log('API Key endpoint not available or key not set');
      return;
    }
    const data = await response.json();
    const apiKeyInput = document.getElementById('api-key');
    if (apiKeyInput && data.key_present) {
      // Don't display the actual key, just indicate it's set
      // Or use a placeholder if preferred
      apiKeyInput.placeholder = 'API Key is set (********)';
      // Optionally, could have a separate status indicator
    }
  } catch (error) {
    console.error('Error fetching API key status:', error);
  }
}

async function handleWebhookSubmit(event) {
  event.preventDefault();
  const form = event.target;
  const webhookInput = document.getElementById('webhook-url');
  const apiKeyInput = document.getElementById('api-key'); // Get API Key input

  let webhookUrl = webhookInput.value.trim();
  let apiKey = apiKeyInput.value.trim(); // Get API Key value

  // Keep URL validation (allow empty URL to clear it)
  if (webhookUrl) {
    if (!webhookUrl.match(/^https?:\/\//i)) {
      webhookUrl = 'http://' + webhookUrl;
    }
    try {
      new URL(webhookUrl);
    } catch (e) {
      showMessage('Please enter a valid URL or leave it empty to clear.', 'error');
      return;
    }
  }

  // Basic check: if URL is provided, API key should also be provided
  if (webhookUrl && !apiKey) {
    showMessage('API Key is required when Webhook URL is set.', 'error');
    apiKeyInput.focus(); // Focus the API key input
    return;
  }

  showMessage('Saving settings...', '');

  // Use Promise.allSettled to send both requests and handle results
  const results = await Promise.allSettled([
    // Request 1: Update Webhook URL (using JSON)
    fetch('/api/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ url: webhookUrl }), // Send empty string to clear
    }),
    // Request 2: Update API Key (using URL-encoded form data)
    fetch('/api/apikey', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `api_key=${encodeURIComponent(apiKey)}`, // Send empty string to potentially clear
    })
  ]);

  // Process results
  const urlResult = results[0];
  const keyResult = results[1];
  let success = true;
  let messages = [];

  if (urlResult.status === 'fulfilled' && urlResult.value.ok) {
    messages.push('Webhook URL updated.');
  } else {
    success = false;
    const errorMsg = urlResult.reason ? urlResult.reason.message : `HTTP ${urlResult.value?.status}`;
    messages.push(`Webhook URL update failed: ${errorMsg}`);
    console.error('Webhook URL update failed:', urlResult.reason || urlResult.value);
  }

  if (keyResult.status === 'fulfilled' && keyResult.value.ok) {
    messages.push('API Key updated.');
    // Clear the input field after successful save for security
    apiKeyInput.value = '';
    apiKeyInput.placeholder = 'API Key is set (********)';
  } else {
    success = false;
    const errorMsg = keyResult.reason ? keyResult.reason.message : `HTTP ${keyResult.value?.status}`;
    messages.push(`API Key update failed: ${errorMsg}`);
    console.error('API Key update failed:', keyResult.reason || keyResult.value);
  }

  showMessage(messages.join('<br>'), success ? 'success' : 'error');
}

// --- Fetch and Render Projects --- 
async function fetchAndRenderProjects() {
  showMessage('', ''); // Clear previous messages
  try {
    const response = await fetch(apiBaseUrl);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json();
    // Handle both array response (old firmware) and object with projects property (new firmware)
    const projects = Array.isArray(data) ? data : (data.projects || []);
    renderProjectList(projects);
  } catch (error) {
    console.error('Error fetching projects:', error);
    renderError('Could not load projects. Is TheTimer connected?');
  }
}

function renderProjectList(projects) {
  if (!projectListDiv) return;

  if (projects.length === 0) {
    projectListDiv.innerHTML = `
      <div class="empty-state">
        <i data-lucide="clipboard-list" style="width: 3rem; height: 3rem; opacity: 0.5; margin-bottom: 1rem;"></i>
        <p>No projects defined yet.</p>
        <p>Create your first project using the form below.</p>
      </div>
    `;
    lucide.createIcons();
    return;
  }

  let tableHtml = `
        <table>
            <thead>
                <tr>
                    <th>Project</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
    `;

  projects.forEach((project, index) => {
    tableHtml += `
            <tr data-index="${index}">
                <td>
                  <span class="color-preview" style="background-color: ${escapeHtml(project.color)};"></span>
                  ${escapeHtml(project.name)}
                </td>
                <td>
                    <button class="btn edit-btn" onclick="handleEditClick(${index})">
                      <i data-lucide="edit-2" class="icon"></i>
                      Edit
                    </button>
                    <button class="btn delete-btn" onclick="handleDeleteClick(${index})">
                      <i data-lucide="trash-2" class="icon"></i>
                      Delete
                    </button>
                </td>
            </tr>
        `;
  });

  tableHtml += `
            </tbody>
        </table>
    `;

  projectListDiv.innerHTML = tableHtml;

  // Initialize Lucide icons in the newly added content
  lucide.createIcons();
}

// --- Add Project --- 
async function handleAddProjectSubmit(event) {
  event.preventDefault();
  const form = event.target;
  const nameInput = document.getElementById('name');

  const name = nameInput.value.trim();
  const color = window.selectedColorValue || '#3b82f6';

  if (!name) {
    showMessage('Project name cannot be empty.', 'error');
    return;
  }

  const newProject = { name, color };
  showMessage('Adding project...', '');

  try {
    const response = await fetch(apiBaseUrl, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(newProject),
    });

    if (response.ok) { // Status 201 Created (or potentially 200 OK)
      showMessage('Project added successfully.', 'success');
      form.reset(); // Clear the form
      window.selectedColorValue = '#3b82f6'; // Reset color to default
      document.getElementById('selectedColor').style.backgroundColor = '#3b82f6';
      document.getElementById('colorPickerDropdown').style.display = 'none';

      fetchAndRenderProjects(); // Refresh the list
    } else {
      const errorData = await response.json().catch(() => ({ error: 'Failed to add project. Invalid response from device.' }));
      throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
    }
  } catch (error) {
    console.error('Error adding project:', error);
    showMessage(`Error: ${error.message}`, 'error');
  }
}

// --- Edit/Delete --- 
function handleEditClick(index) {
  console.log(`Edit clicked for index: ${index}`);
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  if (!row || row.classList.contains('editing')) return; // Prevent editing multiple rows or non-existent rows

  row.classList.add('editing'); // Mark row as being edited

  const nameCell = row.cells[0];
  const actionsCell = row.cells[1];

  // Extract name and color from the first cell
  const colorPreview = nameCell.querySelector('.color-preview');
  const currentName = nameCell.textContent.trim();
  const currentColorHex = colorPreview ? rgbToHex(colorPreview.style.backgroundColor) || colorPreview.style.backgroundColor : '#000000';

  // Store original values for cancellation
  row.dataset.originalName = currentName;
  row.dataset.originalColor = currentColorHex;

  // Store edit state in window for color picker access
  window.editingRow = row;
  window.editingIndex = index;
  window.editingOriginalColor = currentColorHex;
  
  nameCell.innerHTML = `
    <div class="edit-project-wrapper">
      <div class="selected-color" id="editSelectedColor" onclick="toggleEditColorPicker()" style="background-color: ${escapeHtml(currentColorHex)};"></div>
      <input type="text" class="edit-name" id="editName" value="${escapeHtml(currentName)}" required>
    </div>
    <div class="color-picker-dropdown" id="editColorPickerDropdown" style="display: none;">
        <!-- Tailwind 500 Colors -->
        <div class="color-presets">
          <div class="color-preset" data-color="#ef4444" onclick="selectEditColor('#ef4444')" style="background-color: #ef4444;" title="Red 500"></div>
          <div class="color-preset" data-color="#f59e0b" onclick="selectEditColor('#f59e0b')" style="background-color: #f59e0b;" title="Amber 500"></div>
          <div class="color-preset" data-color="#84cc16" onclick="selectEditColor('#84cc16')" style="background-color: #84cc16;" title="Lime 500"></div>
          <div class="color-preset" data-color="#10b981" onclick="selectEditColor('#10b981')" style="background-color: #10b981;" title="Emerald 500"></div>
          <div class="color-preset" data-color="#06b6d4" onclick="selectEditColor('#06b6d4')" style="background-color: #06b6d4;" title="Cyan 500"></div>
          <div class="color-preset" data-color="#3b82f6" onclick="selectEditColor('#3b82f6')" style="background-color: #3b82f6;" title="Blue 500"></div>
          <div class="color-preset" data-color="#8b5cf6" onclick="selectEditColor('#8b5cf6')" style="background-color: #8b5cf6;" title="Violet 500"></div>
          <div class="color-preset" data-color="#d946ef" onclick="selectEditColor('#d946ef')" style="background-color: #d946ef;" title="Fuchsia 500"></div>
          <div class="color-preset" data-color="#f43f5e" onclick="selectEditColor('#f43f5e')" style="background-color: #f43f5e;" title="Rose 500"></div>
        </div>
        
        <!-- Custom Color Section -->
        <div class="custom-color-section">
          <div class="custom-color-header">
            <i data-lucide="edit-3" class="icon"></i>
            <span>Custom Color</span>
          </div>
          <div class="custom-color-picker">
            <div class="color-picker-main">
              <div class="color-preview-large" id="editColorPreviewLarge" style="background-color: ${escapeHtml(currentColorHex)};"></div>
              <div class="color-gradient-picker">
                <div class="hue-saturation-picker" id="editHueSatPicker">
                  <div class="hue-sat-cursor" id="editHueSatCursor"></div>
                </div>
                <div class="hue-slider" id="editHueSlider">
                  <div class="hue-cursor" id="editHueCursor"></div>
                </div>
              </div>
            </div>
            <div class="rgb-inputs">
              <div class="rgb-input-group">
                <label>R</label>
                <input type="number" id="editRInput" min="0" max="255" value="0" onchange="updateEditFromRGB()">
              </div>
              <div class="rgb-input-group">
                <label>G</label>
                <input type="number" id="editGInput" min="0" max="255" value="0" onchange="updateEditFromRGB()">
              </div>
              <div class="rgb-input-group">
                <label>B</label>
                <input type="number" id="editBInput" min="0" max="255" value="0" onchange="updateEditFromRGB()">
              </div>
            </div>
          </div>
        </div>
    </div>
  `;
  
  // Set initial edit color value
  window.editSelectedColorValue = currentColorHex;
  
  // Initialize RGB values for edit
  const rgb = hexToRgb2(currentColorHex);
  if (rgb) {
    document.getElementById('editRInput').value = rgb.r;
    document.getElementById('editGInput').value = rgb.g;
    document.getElementById('editBInput').value = rgb.b;
  }
  actionsCell.innerHTML = `
        <button class="btn save-btn" onclick="handleSaveClick(${index})">
          <i data-lucide="check" class="icon"></i>
          Save
        </button>
        <button class="btn cancel-btn" onclick="handleCancelClick(${index})">
          <i data-lucide="x" class="icon"></i>
          Cancel
        </button>
    `;

  // Initialize Lucide icons in the newly added content
  lucide.createIcons();
}

// Add WebSocket cleanup to handleCancelClick and handleSaveClick
function handleCancelClick(index) {
  console.log(`Cancel clicked for index: ${index}`);
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  if (!row) return;

  // Reset the LED color when cancelling edit
  sendResetColorUpdate();

  // Retrieve original values
  const originalName = row.dataset.originalName;
  const originalColor = row.dataset.originalColor;

  // Revert cells (re-render like in renderProjectList)
  row.cells[0].innerHTML = `
    <span class="color-preview" style="background-color: ${escapeHtml(originalColor)};"></span>
    ${escapeHtml(originalName)}
  `;
  row.cells[1].innerHTML = `
    <button class="btn edit-btn" onclick="handleEditClick(${index})">
      <i data-lucide="edit-2" class="icon"></i>
      Edit
    </button>
    <button class="btn delete-btn" onclick="handleDeleteClick(${index})">
      <i data-lucide="trash-2" class="icon"></i>
      Delete
    </button>
  `;

  // Initialize Lucide icons in the newly added content
  lucide.createIcons();

  row.classList.remove('editing');
  showMessage('Edit cancelled.', '');
}

async function handleSaveClick(index) {
  console.log(`Save clicked for index: ${index}`);
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  if (!row) return;

  // Reset the LED color when saving
  sendResetColorUpdate();

  const nameInput = document.getElementById('editName');
  const newName = nameInput.value.trim();
  const newColor = window.editSelectedColorValue;

  if (!newName) {
    showMessage('Project name cannot be empty.', 'error');
    return;
  }

  // Package index along with name and color for the POST body
  const updatedProjectData = { index: index, name: newName, color: newColor };
  showMessage(`Saving project ${index}...`, '');

  try {
    // Use POST to a dedicated update path
    const response = await fetch(`/api/updateProject`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(updatedProjectData),
    });

    if (response.ok) {
      showMessage('Project updated successfully.', 'success');
      fetchAndRenderProjects();
    } else {
      const errorData = await response.json().catch(() => ({ error: 'Failed to update project.' }));
      if (response.status === 404) { // Check if index was the problem server-side
        throw new Error(errorData.error || `Update endpoint not found or project index ${index} invalid.`);
      } else {
        throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
      }
    }
  } catch (error) {
    console.error('Error updating project:', error);
    showMessage(`Error: ${error.message}`, 'error');
  }
}

// --- Delete --- 
async function handleDeleteClick(index) {
  console.log(`Delete clicked for index: ${index}`);

  // Find the project name for confirmation
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  const projectName = row ? row.cells[0].textContent.trim() : `Project at index ${index}`;

  if (!confirm(`Are you sure you want to delete project "${projectName}"?`)) {
    return; // User cancelled
  }

  showMessage(`Deleting project "${projectName}"...`, '');

  try {
    // Send index in the request body (form-encoded)
    const formData = new URLSearchParams();
    formData.append('index', index);

    const response = await fetch(`/api/deleteProject`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: formData,
    });

    if (response.ok) {
      showMessage(`Project "${projectName}" deleted successfully.`, 'success');
      fetchAndRenderProjects(); // Refresh the list instead of reloading
    } else {
      const errorData = await response.json().catch(() => ({ error: 'Failed to delete project. Invalid response from device.' }));
      if (response.status === 404) {
        throw new Error(`Project not found on device (index ${index}). Maybe it was already deleted?`);
      } else {
        throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
      }
    }
  } catch (error) {
    console.error('Error deleting project:', error);
    showMessage(`Error: ${error.message}`, 'error');
  }
}

// --- Utility Functions --- 
function showMessage(msg, type = '') {
  if (messageArea) {
    messageArea.textContent = msg;

    // Reset all classes
    messageArea.className = '';

    // Add message type class if specified
    if (type === 'success') {
      messageArea.classList.add('success-message');
    } else if (type === 'error') {
      messageArea.classList.add('error-message');
    }
  }
  console.log(msg); // Also log to console
}

function renderError(msg) {
  if (projectListDiv) {
    projectListDiv.innerHTML = `
      <div class="empty-state">
        <i data-lucide="alert-triangle" style="width: 3rem; height: 3rem; color: var(--color-error); margin-bottom: 1rem;"></i>
        <p class="error-message">${escapeHtml(msg)}</p>
      </div>
    `;
    lucide.createIcons();
  }
  showMessage(msg, 'error'); // Show in message area too
}

function escapeHtml(unsafe) {
  if (unsafe === null || unsafe === undefined) return '';
  return unsafe
    .toString()
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#039;");
}

// Helper to convert rgb(r, g, b) back to hex - needed for color input
// Basic version, may not handle all edge cases perfectly
function rgbToHex(rgb) {
  if (!rgb || !rgb.startsWith('rgb')) return null;
  // Extract numbers
  const result = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/.exec(rgb);
  if (!result) return null;
  // Convert each part to hex
  const r = parseInt(result[1], 10).toString(16).padStart(2, '0');
  const g = parseInt(result[2], 10).toString(16).padStart(2, '0');
  const b = parseInt(result[3], 10).toString(16).padStart(2, '0');
  return `#${r}${g}${b}`;
}

// Color picker functionality
function toggleColorPicker() {
  const dropdown = document.getElementById('colorPickerDropdown');
  dropdown.style.display = dropdown.style.display === 'none' ? 'block' : 'none';
  
  // Initialize color picker if it's being shown
  if (dropdown.style.display === 'block') {
    initializeColorPicker();
  }
}

function initializeColorPicker() {
  const hueSatPicker = document.getElementById('hueSatPicker');
  const hueSlider = document.getElementById('hueSlider');
  const hueSatCursor = document.getElementById('hueSatCursor');
  const hueCursor = document.getElementById('hueCursor');
  
  // Initialize cursor positions
  updateHueSatPicker();
  updateHueSlider();
  
  // Add event listeners
  hueSatPicker.addEventListener('mousedown', onHueSatMouseDown);
  hueSlider.addEventListener('mousedown', onHueMouseDown);
}

function onHueSatMouseDown(e) {
  e.preventDefault();
  const rect = e.target.getBoundingClientRect();
  
  function onMouseMove(e) {
    const x = Math.max(0, Math.min(rect.width, e.clientX - rect.left));
    const y = Math.max(0, Math.min(rect.height, e.clientY - rect.top));
    
    window.currentSaturation = (x / rect.width) * 100;
    window.currentLightness = 100 - (y / rect.height) * 100;
    
    updateColorFromHSL();
    updateHueSatCursor(x, y);
  }
  
  function onMouseUp() {
    document.removeEventListener('mousemove', onMouseMove);
    document.removeEventListener('mouseup', onMouseUp);
  }
  
  onMouseMove(e);
  document.addEventListener('mousemove', onMouseMove);
  document.addEventListener('mouseup', onMouseUp);
}

function onHueMouseDown(e) {
  e.preventDefault();
  const rect = e.target.getBoundingClientRect();
  
  function onMouseMove(e) {
    const x = Math.max(0, Math.min(rect.width, e.clientX - rect.left));
    window.currentHue = (x / rect.width) * 360;
    
    updateColorFromHSL();
    updateHueSlider();
    updateHueSatPicker();
  }
  
  function onMouseUp() {
    document.removeEventListener('mousemove', onMouseMove);
    document.removeEventListener('mouseup', onMouseUp);
  }
  
  onMouseMove(e);
  document.addEventListener('mousemove', onMouseMove);
  document.addEventListener('mouseup', onMouseUp);
}

function updateColorFromHSL() {
  const rgb = hslToRgb(window.currentHue / 360, window.currentSaturation / 100, window.currentLightness / 100);
  const hexColor = rgbToHex2(rgb.r, rgb.g, rgb.b);
  
  window.selectedColorValue = hexColor;
  document.getElementById('selectedColor').style.backgroundColor = hexColor;
  document.getElementById('colorPreviewLarge').style.backgroundColor = hexColor;
  
  // Update RGB inputs
  document.getElementById('rInput').value = rgb.r;
  document.getElementById('gInput').value = rgb.g;
  document.getElementById('bInput').value = rgb.b;
  
  // Send color update to device
  sendColorUpdate(hexColor);
  
  // Clear preset selections
  document.querySelectorAll('.color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
}

function updateHueSatCursor(x, y) {
  const cursor = document.getElementById('hueSatCursor');
  if (x !== undefined && y !== undefined) {
    cursor.style.left = x + 'px';
    cursor.style.top = y + 'px';
  } else {
    const picker = document.getElementById('hueSatPicker');
    const rect = picker.getBoundingClientRect();
    const x = (window.currentSaturation / 100) * picker.offsetWidth;
    const y = ((100 - window.currentLightness) / 100) * picker.offsetHeight;
    cursor.style.left = x + 'px';
    cursor.style.top = y + 'px';
  }
}

function updateHueSlider() {
  const cursor = document.getElementById('hueCursor');
  const slider = document.getElementById('hueSlider');
  const x = (window.currentHue / 360) * slider.offsetWidth;
  cursor.style.left = x + 'px';
}

function updateHueSatPicker() {
  const picker = document.getElementById('hueSatPicker');
  const hueColor = `hsl(${window.currentHue}, 100%, 50%)`;
  picker.style.background = `
    linear-gradient(to top, #000, transparent), 
    linear-gradient(to right, #fff, transparent),
    ${hueColor}
  `;
  updateHueSatCursor();
}

function selectColor(color) {
  window.selectedColorValue = color;
  document.getElementById('selectedColor').style.backgroundColor = color;
  document.getElementById('colorPreviewLarge').style.backgroundColor = color;
  
  // Update RGB inputs
  const rgb = hexToRgb2(color);
  if (rgb) {
    document.getElementById('rInput').value = rgb.r;
    document.getElementById('gInput').value = rgb.g;
    document.getElementById('bInput').value = rgb.b;
    
    // Update HSL values
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.currentHue = hsl.h * 360;
    window.currentSaturation = hsl.s * 100;
    window.currentLightness = hsl.l * 100;
    updateHueSatPicker();
    updateHueSlider();
  }
  
  // Send color update to device
  sendColorUpdate(color);
  
  // Clear preset selections and mark this one as selected
  document.querySelectorAll('.color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
  if (document.querySelector(`[data-color="${color}"]`)) {
    document.querySelector(`[data-color="${color}"]`).classList.add('selected');
  }
  
  // Close dropdown
  document.getElementById('colorPickerDropdown').style.display = 'none';
}

function updateFromRGB() {
  const r = parseInt(document.getElementById('rInput').value) || 0;
  const g = parseInt(document.getElementById('gInput').value) || 0;
  const b = parseInt(document.getElementById('bInput').value) || 0;
  
  const hexColor = rgbToHex2(r, g, b);
  window.selectedColorValue = hexColor;
  
  // Convert RGB to HSL for the color picker
  const hsl = rgbToHsl(r, g, b);
  window.currentHue = hsl.h * 360;
  window.currentSaturation = hsl.s * 100;
  window.currentLightness = hsl.l * 100;
  
  document.getElementById('selectedColor').style.backgroundColor = hexColor;
  document.getElementById('colorPreviewLarge').style.backgroundColor = hexColor;
  
  // Update color picker visuals
  updateHueSatPicker();
  updateHueSlider();
  
  // Send color update to device
  sendColorUpdate(hexColor);
  
  // Clear preset selections
  document.querySelectorAll('.color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
}

// Color conversion utilities
function hexToRgb2(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : null;
}

function rgbToHex2(r, g, b) {
  return "#" + [r, g, b].map(x => {
    const hex = x.toString(16);
    return hex.length === 1 ? "0" + hex : hex;
  }).join("");
}

function hslToRgb(h, s, l) {
  let r, g, b;

  if (s === 0) {
    r = g = b = l; // achromatic
  } else {
    const hue2rgb = (p, q, t) => {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1/6) return p + (q - p) * 6 * t;
      if (t < 1/2) return q;
      if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
      return p;
    };

    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    r = hue2rgb(p, q, h + 1/3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1/3);
  }

  return {
    r: Math.round(r * 255),
    g: Math.round(g * 255),
    b: Math.round(b * 255)
  };
}

function rgbToHsl(r, g, b) {
  r /= 255;
  g /= 255;
  b /= 255;
  
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  let h, s, l = (max + min) / 2;

  if (max === min) {
    h = s = 0; // achromatic
  } else {
    const d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    switch (max) {
      case r: h = (g - b) / d + (g < b ? 6 : 0); break;
      case g: h = (b - r) / d + 2; break;
      case b: h = (r - g) / d + 4; break;
    }
    h /= 6;
  }

  return { h, s, l };
}

// Close color picker when clicking outside
document.addEventListener('click', function(event) {
  const colorPicker = document.getElementById('colorPickerDropdown');
  const selectedColor = document.getElementById('selectedColor');
  const editColorPicker = document.getElementById('editColorPickerDropdown');
  const editSelectedColor = document.getElementById('editSelectedColor');
  
  if (colorPicker && selectedColor && !colorPicker.contains(event.target) && event.target !== selectedColor) {
    colorPicker.style.display = 'none';
  }
  
  if (editColorPicker && editSelectedColor && !editColorPicker.contains(event.target) && event.target !== editSelectedColor) {
    editColorPicker.style.display = 'none';
  }
});

// Edit color picker functions
function toggleEditColorPicker() {
  const dropdown = document.getElementById('editColorPickerDropdown');
  dropdown.style.display = dropdown.style.display === 'none' ? 'block' : 'none';
  
  if (dropdown.style.display === 'block') {
    initializeEditColorPicker();
  }
}

function initializeEditColorPicker() {
  const hueSatPicker = document.getElementById('editHueSatPicker');
  const hueSlider = document.getElementById('editHueSlider');
  
  // Initialize cursor positions for current color
  const rgb = hexToRgb2(window.editSelectedColorValue);
  if (rgb) {
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.editCurrentHue = hsl.h * 360;
    window.editCurrentSaturation = hsl.s * 100;
    window.editCurrentLightness = hsl.l * 100;
    updateEditHueSatPicker();
    updateEditHueSlider();
  }
  
  // Add event listeners
  hueSatPicker.addEventListener('mousedown', onEditHueSatMouseDown);
  hueSlider.addEventListener('mousedown', onEditHueMouseDown);
}

function onEditHueSatMouseDown(e) {
  e.preventDefault();
  const rect = e.target.getBoundingClientRect();
  
  function onMouseMove(e) {
    const x = Math.max(0, Math.min(rect.width, e.clientX - rect.left));
    const y = Math.max(0, Math.min(rect.height, e.clientY - rect.top));
    
    window.editCurrentSaturation = (x / rect.width) * 100;
    window.editCurrentLightness = 100 - (y / rect.height) * 100;
    
    updateEditColorFromHSL();
    updateEditHueSatCursor(x, y);
  }
  
  function onMouseUp() {
    document.removeEventListener('mousemove', onMouseMove);
    document.removeEventListener('mouseup', onMouseUp);
  }
  
  onMouseMove(e);
  document.addEventListener('mousemove', onMouseMove);
  document.addEventListener('mouseup', onMouseUp);
}

function onEditHueMouseDown(e) {
  e.preventDefault();
  const rect = e.target.getBoundingClientRect();
  
  function onMouseMove(e) {
    const x = Math.max(0, Math.min(rect.width, e.clientX - rect.left));
    window.editCurrentHue = (x / rect.width) * 360;
    
    updateEditColorFromHSL();
    updateEditHueSlider();
    updateEditHueSatPicker();
  }
  
  function onMouseUp() {
    document.removeEventListener('mousemove', onMouseMove);
    document.removeEventListener('mouseup', onMouseUp);
  }
  
  onMouseMove(e);
  document.addEventListener('mousemove', onMouseMove);
  document.addEventListener('mouseup', onMouseUp);
}

function updateEditColorFromHSL() {
  const rgb = hslToRgb(window.editCurrentHue / 360, window.editCurrentSaturation / 100, window.editCurrentLightness / 100);
  const hexColor = rgbToHex2(rgb.r, rgb.g, rgb.b);
  
  window.editSelectedColorValue = hexColor;
  document.getElementById('editSelectedColor').style.backgroundColor = hexColor;
  document.getElementById('editColorPreviewLarge').style.backgroundColor = hexColor;
  
  // Update RGB inputs
  document.getElementById('editRInput').value = rgb.r;
  document.getElementById('editGInput').value = rgb.g;
  document.getElementById('editBInput').value = rgb.b;
  
  // Send color update to device
  sendColorUpdate(hexColor);
  
  // Clear preset selections
  document.querySelectorAll('.color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
}

function updateEditHueSatCursor(x, y) {
  const cursor = document.getElementById('editHueSatCursor');
  if (x !== undefined && y !== undefined) {
    cursor.style.left = x + 'px';
    cursor.style.top = y + 'px';
  } else {
    const picker = document.getElementById('editHueSatPicker');
    const x = (window.editCurrentSaturation / 100) * picker.offsetWidth;
    const y = ((100 - window.editCurrentLightness) / 100) * picker.offsetHeight;
    cursor.style.left = x + 'px';
    cursor.style.top = y + 'px';
  }
}

function updateEditHueSlider() {
  const cursor = document.getElementById('editHueCursor');
  const slider = document.getElementById('editHueSlider');
  const x = (window.editCurrentHue / 360) * slider.offsetWidth;
  cursor.style.left = x + 'px';
}

function updateEditHueSatPicker() {
  const picker = document.getElementById('editHueSatPicker');
  const hueColor = `hsl(${window.editCurrentHue}, 100%, 50%)`;
  picker.style.background = `
    linear-gradient(to top, #000, transparent), 
    linear-gradient(to right, #fff, transparent),
    ${hueColor}
  `;
  updateEditHueSatCursor();
}

function selectEditColor(color) {
  window.editSelectedColorValue = color;
  document.getElementById('editSelectedColor').style.backgroundColor = color;
  document.getElementById('editColorPreviewLarge').style.backgroundColor = color;
  
  // Update RGB inputs
  const rgb = hexToRgb2(color);
  if (rgb) {
    document.getElementById('editRInput').value = rgb.r;
    document.getElementById('editGInput').value = rgb.g;
    document.getElementById('editBInput').value = rgb.b;
    
    // Update HSL values
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.editCurrentHue = hsl.h * 360;
    window.editCurrentSaturation = hsl.s * 100;
    window.editCurrentLightness = hsl.l * 100;
    updateEditHueSatPicker();
    updateEditHueSlider();
  }
  
  // Send color update to device
  sendColorUpdate(color);
  
  // Clear preset selections and mark this one as selected
  document.querySelectorAll('#editColorPickerDropdown .color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
  document.querySelector(`#editColorPickerDropdown [data-color="${color}"]`).classList.add('selected');
  
  // Close dropdown
  document.getElementById('editColorPickerDropdown').style.display = 'none';
}

function updateEditFromRGB() {
  const r = parseInt(document.getElementById('editRInput').value) || 0;
  const g = parseInt(document.getElementById('editGInput').value) || 0;
  const b = parseInt(document.getElementById('editBInput').value) || 0;
  
  const hexColor = rgbToHex2(r, g, b);
  window.editSelectedColorValue = hexColor;
  
  // Convert RGB to HSL for the color picker
  const hsl = rgbToHsl(r, g, b);
  window.editCurrentHue = hsl.h * 360;
  window.editCurrentSaturation = hsl.s * 100;
  window.editCurrentLightness = hsl.l * 100;
  
  document.getElementById('editSelectedColor').style.backgroundColor = hexColor;
  document.getElementById('editColorPreviewLarge').style.backgroundColor = hexColor;
  
  // Update color picker visuals
  updateEditHueSatPicker();
  updateEditHueSlider();
  
  // Send color update to device
  sendColorUpdate(hexColor);
  
  // Clear preset selections
  document.querySelectorAll('#editColorPickerDropdown .color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
}

// Dropdown functionality
function initDropdown(triggerId, menuId, itemsId, inputId) {
  const trigger = document.getElementById(triggerId);
  const menu = document.getElementById(menuId);
  const items = document.getElementById(itemsId);
  const input = document.getElementById(inputId);
  const valueSpan = trigger.querySelector('.dropdown-value');
  
  // Toggle dropdown
  trigger.addEventListener('click', (e) => {
    e.preventDefault();
    const isOpen = menu.classList.contains('open');
    
    // Close all other dropdowns
    document.querySelectorAll('.dropdown-menu.open').forEach(m => {
      if (m !== menu) m.classList.remove('open');
    });
    document.querySelectorAll('.dropdown-trigger[aria-expanded="true"]').forEach(t => {
      if (t !== trigger) t.setAttribute('aria-expanded', 'false');
    });
    
    menu.classList.toggle('open');
    trigger.setAttribute('aria-expanded', !isOpen);
  });
  
  // Handle item selection
  items.addEventListener('click', (e) => {
    const item = e.target.closest('.dropdown-item');
    if (item) {
      const value = item.dataset.value;
      const text = item.textContent;
      
      // Update display
      valueSpan.textContent = text;
      valueSpan.dataset.value = value;
      input.value = value;
      
      // Update selected state
      items.querySelectorAll('.dropdown-item').forEach(i => i.classList.remove('selected'));
      item.classList.add('selected');
      
      // Close dropdown
      menu.classList.remove('open');
      trigger.setAttribute('aria-expanded', 'false');
      
      // Enable preview button
      if (value) {
        document.getElementById('preview-sound').disabled = false;
      }
    }
  });
  
  // Close on outside click
  document.addEventListener('click', (e) => {
    if (!trigger.contains(e.target) && !menu.contains(e.target)) {
      menu.classList.remove('open');
      trigger.setAttribute('aria-expanded', 'false');
    }
  });
}

// Alarm Settings Functions
async function fetchAlarmSettings() {
  try {
    // Fetch available sounds
    const soundsResponse = await fetch('/api/alarm/sounds');
    if (soundsResponse.ok) {
      const sounds = await soundsResponse.json();
      const soundItems = document.getElementById('alarm-sound-items');
      soundItems.innerHTML = '';
      
      sounds.forEach(sound => {
        const item = document.createElement('div');
        item.className = 'dropdown-item';
        item.dataset.value = sound;
        item.textContent = sound.replace('.wav', '').replace(/-/g, ' ').replace(/_/g, ' ');
        soundItems.appendChild(item);
      });
      
      // Initialize dropdown
      initDropdown('alarm-sound-trigger', 'alarm-sound-menu', 'alarm-sound-items', 'alarm-sound');
    }

    // Fetch current settings
    const settingsResponse = await fetch('/api/alarm/settings');
    if (settingsResponse.ok) {
      const settings = await settingsResponse.json();
      
      // Set selected sound
      if (settings.sound) {
        const trigger = document.getElementById('alarm-sound-trigger');
        const valueSpan = trigger.querySelector('.dropdown-value');
        const input = document.getElementById('alarm-sound');
        const items = document.getElementById('alarm-sound-items');
        
        valueSpan.textContent = settings.sound.replace('.wav', '').replace(/-/g, ' ').replace(/_/g, ' ');
        valueSpan.dataset.value = settings.sound;
        input.value = settings.sound;
        
        // Mark as selected
        items.querySelectorAll('.dropdown-item').forEach(item => {
          if (item.dataset.value === settings.sound) {
            item.classList.add('selected');
          }
        });
        
        document.getElementById('preview-sound').disabled = false;
      }
      
      document.getElementById('alarm-enabled').checked = settings.enabled !== false;
    }
  } catch (error) {
    console.error('Error fetching alarm settings:', error);
  }
}

async function handleAlarmSubmit(event) {
  event.preventDefault();
  
  const formData = new FormData(event.target);
  const sound = formData.get('alarm-sound');
  const enabled = formData.get('alarm-enabled') === 'on';
  
  try {
    const response = await fetch('/api/alarm/settings', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ sound, enabled })
    });

    if (response.ok) {
      showMessage('Alarm settings saved successfully', 'success');
    } else {
      showMessage('Failed to save alarm settings', 'error');
    }
  } catch (error) {
    console.error('Error saving alarm settings:', error);
    showMessage('Error saving alarm settings', 'error');
  }
}

let previewTimeout = null;

async function handlePreviewSound() {
  const sound = document.getElementById('alarm-sound').value;
  if (!sound) {
    showMessage('Please select a sound first', 'warning');
    return;
  }

  const previewButton = document.getElementById('preview-sound');
  const stopButton = document.getElementById('stop-sound');
  
  previewButton.style.display = 'none';
  stopButton.style.display = 'block';

  try {
    const response = await fetch('/api/alarm/preview', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ sound })
    });

    if (response.ok) {
      // Sound will play until manually stopped
    } else {
      stopButton.style.display = 'none';
      previewButton.style.display = 'block';
      showMessage('Failed to preview sound', 'error');
    }
  } catch (error) {
    console.error('Error previewing sound:', error);
    stopButton.style.display = 'none';
    previewButton.style.display = 'block';
    showMessage('Error previewing sound', 'error');
  }
}

async function handleStopSound() {
  if (previewTimeout) {
    clearTimeout(previewTimeout);
    previewTimeout = null;
  }

  try {
    await fetch('/api/alarm/stop', {
      method: 'POST'
    });
  } catch (error) {
    console.error('Error stopping sound:', error);
  }

  const previewButton = document.getElementById('preview-sound');
  const stopButton = document.getElementById('stop-sound');
  stopButton.style.display = 'none';
  previewButton.style.display = 'block';
}