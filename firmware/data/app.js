// TheTimer Project Management app.js

// Color preview timeout for debouncing
let colorPreviewTimeout = null;

document.addEventListener('DOMContentLoaded', () => {
  console.log('DOM fully loaded');
  
  // Initialize lucide icons
  lucide.createIcons();
  
  fetchAndRenderProjects();
  fetchAlarmSettings();

  const form = document.getElementById('add-project-form');
  if (form) {
    form.addEventListener('submit', handleAddProjectSubmit);
  }

  // Initialize OAuth functionality
  initializeOAuth();

  // Auto-save alarm settings on change
  const alarmSound = document.getElementById('alarm-sound');
  if (alarmSound) {
    // Sound dropdown already handled by initDropdown
  }
  
  const alarmEnabled = document.getElementById('alarm-enabled');
  if (alarmEnabled) {
    alarmEnabled.addEventListener('change', handleAlarmSettingChange);
  }

  const previewButton = document.getElementById('preview-sound');
  if (previewButton) {
    previewButton.addEventListener('click', handlePreviewToggle);
  }

  // Initialize color picker with default color
  window.selectedColorValue = '#3b82f6';
  window.currentHue = 220;
  window.currentSaturation = 100;
  window.currentLightness = 62;
  
  // Initialize hamburger menu
  const hamburgerMenu = document.getElementById('hamburgerMenu');
  const mobileMenu = document.getElementById('mobileMenu');
  
  if (hamburgerMenu && mobileMenu) {
    hamburgerMenu.addEventListener('click', () => {
      mobileMenu.classList.toggle('active');
      const icon = hamburgerMenu.querySelector('i');
      if (mobileMenu.classList.contains('active')) {
        icon.setAttribute('data-lucide', 'x');
      } else {
        icon.setAttribute('data-lucide', 'menu');
      }
      lucide.createIcons();
    });
    
    // Close mobile menu when clicking outside
    document.addEventListener('click', (event) => {
      if (!hamburgerMenu.contains(event.target) && !mobileMenu.contains(event.target)) {
        mobileMenu.classList.remove('active');
        const icon = hamburgerMenu.querySelector('i');
        icon.setAttribute('data-lucide', 'menu');
        lucide.createIcons();
      }
    });
    
    // Close mobile menu when clicking on a link
    const mobileLinks = mobileMenu.querySelectorAll('.navbar-link');
    mobileLinks.forEach(link => {
      link.addEventListener('click', () => {
        mobileMenu.classList.remove('active');
        const icon = hamburgerMenu.querySelector('i');
        icon.setAttribute('data-lucide', 'menu');
        lucide.createIcons();
      });
    });
  }

  // Initialize theme
  initializeTheme();
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

// --- OAuth Configuration ---
const OAUTH_CONFIG = {
  clientId: 'timer-device-client',
  authorizationUrl: 'https://thetimer.app/oauth/authorize',
  tokenUrl: 'https://thetimer.app/oauth/token',
  redirectUri: window.location.origin + '/oauth/callback',
  scope: 'projects:read projects:write timers:write user:read'
};

let oauthState = null;
let oauthVerifier = null;
let oauthPopup = null;

// Initialize OAuth functionality
function initializeOAuth() {
  // Check if we have stored OAuth token
  checkOAuthConnection();
  
  // Set up event listeners
  const connectBtn = document.getElementById('oauth-connect-btn');
  if (connectBtn) {
    connectBtn.addEventListener('click', handleOAuthConnect);
  }
  
  const disconnectBtn = document.getElementById('oauth-disconnect-btn');
  if (disconnectBtn) {
    disconnectBtn.addEventListener('click', handleOAuthDisconnect);
  }
  
  // Listen for OAuth callback messages
  window.addEventListener('message', handleOAuthMessage);
}

// Check OAuth connection status
async function checkOAuthConnection() {
  try {
    const response = await fetch('/api/oauth/status');
    if (response.ok) {
      const data = await response.json();
      if (data.connected) {
        showOAuthState('connected');
        // Load user info
        if (data.user) {
          document.getElementById('oauth-account-name').textContent = data.user.name || 'Unknown';
          document.getElementById('oauth-account-email').textContent = data.user.email || 'Unknown';
        }
      } else {
        showOAuthState('disconnected');
      }
    } else {
      showOAuthState('disconnected');
    }
  } catch (error) {
    console.error('Error checking OAuth status:', error);
    showOAuthState('disconnected');
  }
}

// Handle OAuth connect button click
function handleOAuthConnect() {
  // Generate PKCE challenge
  oauthVerifier = generateCodeVerifier();
  const challenge = generateCodeChallenge(oauthVerifier);
  
  // Generate state for CSRF protection
  oauthState = generateRandomString(32);
  
  // Build authorization URL
  const params = new URLSearchParams({
    client_id: OAUTH_CONFIG.clientId,
    redirect_uri: OAUTH_CONFIG.redirectUri,
    response_type: 'code',
    scope: OAUTH_CONFIG.scope,
    state: oauthState,
    code_challenge: challenge,
    code_challenge_method: 'S256'
  });
  
  const authUrl = `${OAUTH_CONFIG.authorizationUrl}?${params.toString()}`;
  
  // Show connecting state
  showOAuthState('connecting');
  
  // Open OAuth popup
  const width = 600;
  const height = 700;
  const left = (window.screen.width - width) / 2;
  const top = (window.screen.height - height) / 2;
  
  oauthPopup = window.open(
    authUrl,
    'OAuth Login',
    `width=${width},height=${height},left=${left},top=${top},toolbar=no,menubar=no`
  );
  
  // Check popup status
  const popupInterval = setInterval(() => {
    if (oauthPopup && oauthPopup.closed) {
      clearInterval(popupInterval);
      // If popup was closed without completing OAuth, revert to disconnected state
      if (document.getElementById('oauth-connecting').style.display !== 'none') {
        showOAuthState('disconnected');
      }
    }
  }, 1000);
}

// Handle OAuth disconnect
async function handleOAuthDisconnect() {
  try {
    const response = await fetch('/api/oauth/disconnect', {
      method: 'POST'
    });
    
    if (response.ok) {
      showOAuthState('disconnected');
      showMessage('Disconnected from TheTimer.app', 'success');
    } else {
      showMessage('Failed to disconnect from TheTimer.app', 'error');
    }
  } catch (error) {
    console.error('Error disconnecting OAuth:', error);
    showMessage('Error disconnecting from TheTimer.app', 'error');
  }
}

// Handle OAuth callback messages
function handleOAuthMessage(event) {
  // Validate origin
  if (event.origin !== window.location.origin) {
    return;
  }
  
  // Handle OAuth callback
  if (event.data.type === 'oauth-callback') {
    if (event.data.state !== oauthState) {
      showMessage('OAuth authentication failed: Invalid state', 'error');
      showOAuthState('disconnected');
      return;
    }
    
    if (event.data.error) {
      showMessage(`OAuth authentication failed: ${event.data.error}`, 'error');
      showOAuthState('disconnected');
      return;
    }
    
    // Exchange code for token
    exchangeCodeForToken(event.data.code);
  }
}

// Exchange authorization code for access token
async function exchangeCodeForToken(code) {
  try {
    const response = await fetch('/api/oauth/token', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        code: code,
        code_verifier: oauthVerifier,
        redirect_uri: OAUTH_CONFIG.redirectUri
      })
    });
    
    if (response.ok) {
      const data = await response.json();
      handleOAuthSuccess(data);
    } else {
      showMessage('Failed to complete OAuth authentication', 'error');
      showOAuthState('disconnected');
    }
  } catch (error) {
    console.error('Error exchanging code for token:', error);
    showMessage('Error completing OAuth authentication', 'error');
    showOAuthState('disconnected');
  }
}

// Handle successful OAuth
function handleOAuthSuccess(data) {
  // Show connected state
  showOAuthState('connected');
  
  // Load user info
  if (data.user) {
    document.getElementById('oauth-account-name').textContent = data.user.name || 'Unknown';
    document.getElementById('oauth-account-email').textContent = data.user.email || 'Unknown';
  }
  
  showMessage('Successfully connected to TheTimer.app', 'success');
  
  // Refresh projects to get synced data
  fetchAndRenderProjects();
}

// Show OAuth state
function showOAuthState(state) {
  const states = ['disconnected', 'connecting', 'connected'];
  states.forEach(s => {
    const element = document.getElementById(`oauth-${s}`);
    if (element) {
      element.style.display = s === state ? 'block' : 'none';
    }
  });
  
  // Update lucide icons
  lucide.createIcons();
}

// PKCE helper functions
function generateRandomString(length) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~';
  let result = '';
  for (let i = 0; i < length; i++) {
    result += chars.charAt(Math.floor(Math.random() * chars.length));
  }
  return result;
}

function generateCodeVerifier() {
  return generateRandomString(128);
}

function generateCodeChallenge(verifier) {
  // In production, this would use SHA256 and base64url encoding
  // For now, return a simple hash
  return btoa(verifier).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
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
        <i data-lucide="folder-open" style="width: 3rem; height: 3rem; opacity: 0.3; margin-bottom: 1rem;"></i>
        <p>No projects yet</p>
        <p style="font-size: 0.875rem; opacity: 0.7;">Click "Add New Project" below to get started</p>
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
      
      // Close the add project form
      toggleAddProject();
      
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
    messageArea.innerHTML = msg; // Use innerHTML to support line breaks
    messageArea.className = '';

    if (type === 'success') {
      messageArea.classList.add('success-message');
    } else if (type === 'error') {
      messageArea.classList.add('error-message');
    }
  }
  console.log(msg);
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
  showMessage(msg, 'error');
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

function rgbToHex(rgb) {
  if (!rgb || !rgb.startsWith('rgb')) return null;
  const result = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/.exec(rgb);
  if (!result) return null;
  const r = parseInt(result[1], 10).toString(16).padStart(2, '0');
  const g = parseInt(result[2], 10).toString(16).padStart(2, '0');
  const b = parseInt(result[3], 10).toString(16).padStart(2, '0');
  return `#${r}${g}${b}`;
}

// Toggle add project form
function toggleAddProject() {
  const form = document.getElementById('add-project-form');
  const toggle = document.getElementById('addProjectToggle');
  
  if (form.style.display === 'none') {
    form.style.display = 'block';
    toggle.classList.add('expanded');
    // Focus on the name input for better UX
    setTimeout(() => {
      document.getElementById('name').focus();
    }, 100);
  } else {
    form.style.display = 'none';
    toggle.classList.remove('expanded');
    // Reset form when closing
    form.reset();
    window.selectedColorValue = '#3b82f6';
    document.getElementById('selectedColor').style.backgroundColor = '#3b82f6';
    document.getElementById('colorPickerDropdown').style.display = 'none';
  }
  
  // Re-initialize icons
  lucide.createIcons();
}

// Color picker functionality
function toggleColorPicker() {
  const dropdown = document.getElementById('colorPickerDropdown');
  dropdown.style.display = dropdown.style.display === 'none' ? 'block' : 'none';
  
  if (dropdown.style.display === 'block') {
    initializeColorPicker();
  }
}

function initializeColorPicker() {
  const hueSatPicker = document.getElementById('hueSatPicker');
  const hueSlider = document.getElementById('hueSlider');
  const hueSatCursor = document.getElementById('hueSatCursor');
  const hueCursor = document.getElementById('hueCursor');
  
  updateHueSatPicker();
  updateHueSlider();
  
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
  
  document.getElementById('rInput').value = rgb.r;
  document.getElementById('gInput').value = rgb.g;
  document.getElementById('bInput').value = rgb.b;
  
  sendColorUpdate(hexColor);
  
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
  
  const rgb = hexToRgb2(color);
  if (rgb) {
    document.getElementById('rInput').value = rgb.r;
    document.getElementById('gInput').value = rgb.g;
    document.getElementById('bInput').value = rgb.b;
    
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.currentHue = hsl.h * 360;
    window.currentSaturation = hsl.s * 100;
    window.currentLightness = hsl.l * 100;
    updateHueSatPicker();
    updateHueSlider();
  }
  
  sendColorUpdate(color);
  
  document.querySelectorAll('.color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
  if (document.querySelector(`[data-color="${color}"]`)) {
    document.querySelector(`[data-color="${color}"]`).classList.add('selected');
  }
  
  document.getElementById('colorPickerDropdown').style.display = 'none';
}

function updateFromRGB() {
  const r = parseInt(document.getElementById('rInput').value) || 0;
  const g = parseInt(document.getElementById('gInput').value) || 0;
  const b = parseInt(document.getElementById('bInput').value) || 0;
  
  const hexColor = rgbToHex2(r, g, b);
  window.selectedColorValue = hexColor;
  
  const hsl = rgbToHsl(r, g, b);
  window.currentHue = hsl.h * 360;
  window.currentSaturation = hsl.s * 100;
  window.currentLightness = hsl.l * 100;
  
  document.getElementById('selectedColor').style.backgroundColor = hexColor;
  document.getElementById('colorPreviewLarge').style.backgroundColor = hexColor;
  
  updateHueSatPicker();
  updateHueSlider();
  
  sendColorUpdate(hexColor);
  
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
    r = g = b = l;
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
    h = s = 0;
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
  
  const rgb = hexToRgb2(window.editSelectedColorValue);
  if (rgb) {
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.editCurrentHue = hsl.h * 360;
    window.editCurrentSaturation = hsl.s * 100;
    window.editCurrentLightness = hsl.l * 100;
    updateEditHueSatPicker();
    updateEditHueSlider();
  }
  
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
  
  document.getElementById('editRInput').value = rgb.r;
  document.getElementById('editGInput').value = rgb.g;
  document.getElementById('editBInput').value = rgb.b;
  
  sendColorUpdate(hexColor);
  
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
  
  const rgb = hexToRgb2(color);
  if (rgb) {
    document.getElementById('editRInput').value = rgb.r;
    document.getElementById('editGInput').value = rgb.g;
    document.getElementById('editBInput').value = rgb.b;
    
    const hsl = rgbToHsl(rgb.r, rgb.g, rgb.b);
    window.editCurrentHue = hsl.h * 360;
    window.editCurrentSaturation = hsl.s * 100;
    window.editCurrentLightness = hsl.l * 100;
    updateEditHueSatPicker();
    updateEditHueSlider();
  }
  
  sendColorUpdate(color);
  
  document.querySelectorAll('#editColorPickerDropdown .color-preset').forEach(preset => {
    preset.classList.remove('selected');
  });
  document.querySelector(`#editColorPickerDropdown [data-color="${color}"]`).classList.add('selected');
  
  document.getElementById('editColorPickerDropdown').style.display = 'none';
}

function updateEditFromRGB() {
  const r = parseInt(document.getElementById('editRInput').value) || 0;
  const g = parseInt(document.getElementById('editGInput').value) || 0;
  const b = parseInt(document.getElementById('editBInput').value) || 0;
  
  const hexColor = rgbToHex2(r, g, b);
  window.editSelectedColorValue = hexColor;
  
  const hsl = rgbToHsl(r, g, b);
  window.editCurrentHue = hsl.h * 360;
  window.editCurrentSaturation = hsl.s * 100;
  window.editCurrentLightness = hsl.l * 100;
  
  document.getElementById('editSelectedColor').style.backgroundColor = hexColor;
  document.getElementById('editColorPreviewLarge').style.backgroundColor = hexColor;
  
  updateEditHueSatPicker();
  updateEditHueSlider();
  
  sendColorUpdate(hexColor);
  
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
  
  trigger.addEventListener('click', (e) => {
    e.preventDefault();
    const isOpen = menu.classList.contains('open');
    
    document.querySelectorAll('.dropdown-menu.open').forEach(m => {
      if (m !== menu) m.classList.remove('open');
    });
    document.querySelectorAll('.dropdown-trigger[aria-expanded="true"]').forEach(t => {
      if (t !== trigger) t.setAttribute('aria-expanded', 'false');
    });
    
    menu.classList.toggle('open');
    trigger.setAttribute('aria-expanded', !isOpen);
  });
  
  items.addEventListener('click', (e) => {
    const item = e.target.closest('.dropdown-item');
    if (item) {
      const value = item.dataset.value;
      const text = item.textContent;
      
      valueSpan.textContent = text;
      valueSpan.dataset.value = value;
      input.value = value;
      
      items.querySelectorAll('.dropdown-item').forEach(i => i.classList.remove('selected'));
      item.classList.add('selected');
      
      menu.classList.remove('open');
      trigger.setAttribute('aria-expanded', 'false');
      
      if (value) {
        document.getElementById('preview-sound').disabled = false;
      }
      
      if (triggerId === 'alarm-sound-trigger') {
        handleAlarmSettingChange();
      }
    }
  });
  
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

async function handleAlarmSettingChange() {
  const sound = document.getElementById('alarm-sound').value;
  const enabled = document.getElementById('alarm-enabled').checked;
  
  try {
    const response = await fetch('/api/alarm/settings', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ sound, enabled })
    });

    if (!response.ok) {
      console.error('Failed to save alarm settings');
    }
  } catch (error) {
    console.error('Error saving alarm settings:', error);
  }
}

let previewTimeout = null;
let isPlaying = false;

async function handlePreviewToggle() {
  const button = document.getElementById('preview-sound');
  const icon = button.querySelector('.icon');
  const text = button.querySelector('span');
  
  if (!isPlaying) {
    const sound = document.getElementById('alarm-sound').value;
    if (!sound) {
      showMessage('Please select a sound first', 'warning');
      return;
    }
    
    isPlaying = true;
    button.classList.remove('secondary-btn');
    button.classList.add('danger-btn');
    icon.setAttribute('data-lucide', 'square');
    text.textContent = 'Stop';
    lucide.createIcons();

    try {
      const response = await fetch('/api/alarm/preview', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ sound })
      });

      if (!response.ok) {
        isPlaying = false;
        button.classList.remove('danger-btn');
        button.classList.add('secondary-btn');
        icon.setAttribute('data-lucide', 'play');
        text.textContent = 'Preview';
        lucide.createIcons();
        showMessage('Failed to preview sound', 'error');
      }
    } catch (error) {
      console.error('Error previewing sound:', error);
      isPlaying = false;
      button.classList.remove('danger-btn');
      button.classList.add('secondary-btn');
      icon.setAttribute('data-lucide', 'play');
      text.textContent = 'Preview';
      lucide.createIcons();
      showMessage('Error previewing sound', 'error');
    }
  } else {
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

    isPlaying = false;
    button.classList.remove('danger-btn');
    button.classList.add('secondary-btn');
    icon.setAttribute('data-lucide', 'play');
    text.textContent = 'Preview';
    lucide.createIcons();
  }
}

// Theme switching functionality
function initializeTheme() {
  const themeToggle = document.getElementById('themeToggle');
  const body = document.body;
  
  // Check for saved theme preference or default to dark mode
  const currentTheme = localStorage.getItem('theme') || 'dark';
  if (currentTheme === 'light') {
    body.classList.add('light-mode');
  }
  
  if (themeToggle) {
    themeToggle.addEventListener('click', () => {
      body.classList.toggle('light-mode');
      const theme = body.classList.contains('light-mode') ? 'light' : 'dark';
      localStorage.setItem('theme', theme);
    });
  }
}