// Focus Dial Project Management app.js

document.addEventListener('DOMContentLoaded', () => {
  console.log('DOM fully loaded');
  fetchAndRenderProjects();
  fetchWebhookUrl();

  const form = document.getElementById('add-project-form');
  if (form) {
    form.addEventListener('submit', handleAddProjectSubmit);
  }

  const webhookForm = document.getElementById('webhook-form');
  if (webhookForm) {
    webhookForm.addEventListener('submit', handleWebhookSubmit);
  }

  // Setup color input and hex value display
  const colorInput = document.getElementById('color');
  const colorHexValue = document.getElementById('color-hex-value');
  if (colorInput && colorHexValue) {
    // Initial value
    colorHexValue.textContent = colorInput.value;

    // Update hex value when color changes
    colorInput.addEventListener('input', () => {
      colorHexValue.textContent = colorInput.value;
    });
  }
});

const apiBaseUrl = '/api/projects';
const projectListDiv = document.getElementById('project-list');
const messageArea = document.getElementById('message-area');

// --- Webhook URL Configuration ---
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

async function handleWebhookSubmit(event) {
  event.preventDefault();
  const form = event.target;
  const webhookInput = document.getElementById('webhook-url');

  let webhookUrl = webhookInput.value.trim();

  if (!webhookUrl) {
    showMessage('Webhook URL cannot be empty.', 'error');
    return;
  }

  // Ensure URL has a protocol
  if (!webhookUrl.match(/^https?:\/\//i)) {
    webhookUrl = 'http://' + webhookUrl;
  }

  // Simple URL validation
  try {
    new URL(webhookUrl); // Will throw if invalid URL
  } catch (e) {
    showMessage('Please enter a valid URL.', 'error');
    return;
  }

  showMessage('Updating webhook URL...', '');

  try {
    const response = await fetch('/api/webhook', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ url: webhookUrl }),
    });

    if (response.ok) {
      showMessage('Webhook URL updated successfully.', 'success');
    } else {
      const errorData = await response.json().catch(() => ({ error: 'Failed to update webhook URL. Invalid response from device.' }));
      throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
    }
  } catch (error) {
    console.error('Error updating webhook URL:', error);
    showMessage(`Error: ${error.message}`, 'error');
  }
}

// --- Fetch and Render Projects --- 
async function fetchAndRenderProjects() {
  showMessage('', ''); // Clear previous messages
  try {
    const response = await fetch(apiBaseUrl);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const projects = await response.json();
    renderProjectList(projects);
  } catch (error) {
    console.error('Error fetching projects:', error);
    renderError('Could not load projects. Is the Focus Dial connected?');
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
                    <th>Name</th>
                    <th>Color</th>
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
                  <code class="color-hex">${escapeHtml(project.color)}</code>
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
  const colorInput = document.getElementById('color');

  const name = nameInput.value.trim();
  const color = colorInput.value;

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
      colorInput.value = '#0070f3'; // Reset color picker to default

      // Also reset the color hex value
      const colorHexValue = document.getElementById('color-hex-value');
      if (colorHexValue) {
        colorHexValue.textContent = '#0070f3';
      }

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
  const colorCell = row.cells[1];
  const actionsCell = row.cells[2];

  // Extract name from the first cell (removing the color preview)
  const currentName = nameCell.textContent.trim();

  // Get color from the second cell (hex code)
  const currentColorHex = colorCell.querySelector('.color-hex').textContent.trim();

  // Store original values for cancellation
  row.dataset.originalName = currentName;
  row.dataset.originalColor = currentColorHex;

  nameCell.innerHTML = `<input type="text" class="edit-name" value="${escapeHtml(currentName)}" required>`;
  colorCell.innerHTML = `
    <div class="color-input-wrapper">
      <input type="color" class="edit-color" value="${escapeHtml(currentColorHex)}" required>
      <span class="color-hex edit-color-hex">${escapeHtml(currentColorHex)}</span>
    </div>
  `;
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

  // Add event listener for color preview
  const editColorInput = colorCell.querySelector('.edit-color');
  const editColorHex = colorCell.querySelector('.edit-color-hex');
  if (editColorInput && editColorHex) {
    editColorInput.addEventListener('input', () => {
      editColorHex.textContent = editColorInput.value;
    });
  }
}

async function handleSaveClick(index) {
  console.log(`Save clicked for index: ${index}`);
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  if (!row) return;

  const nameInput = row.querySelector('.edit-name');
  const colorInput = row.querySelector('.edit-color');

  const newName = nameInput.value.trim();
  const newColor = colorInput.value;

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

function handleCancelClick(index) {
  console.log(`Cancel clicked for index: ${index}`);
  const row = projectListDiv.querySelector(`tr[data-index="${index}"]`);
  if (!row) return;

  // Retrieve original values
  const originalName = row.dataset.originalName;
  const originalColor = row.dataset.originalColor;

  // Revert cells (re-render like in renderProjectList)
  row.cells[0].innerHTML = `
    <span class="color-preview" style="background-color: ${escapeHtml(originalColor)};"></span>
    ${escapeHtml(originalName)}
  `;
  row.cells[1].innerHTML = `
    <code class="color-hex">${escapeHtml(originalColor)}</code>
  `;
  row.cells[2].innerHTML = `
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