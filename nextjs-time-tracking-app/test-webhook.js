const axios = require('axios');

// Configuration
const BASE_URL = 'http://localhost:3004';
const WEBHOOK_URL = `${BASE_URL}/api/webhook`;

// Generate a unique device project ID
const deviceProjectId = `test-project-${Date.now()}`;

// Function to send webhook requests
async function sendWebhook(event, projectName) {
  const payload = {
    event,
    device_id: 'test-device-001',
    timestamp: new Date().toISOString(),
    project_name: projectName,
    project_color: '#FF5733',
    device_project_id: deviceProjectId
  };

  console.log(`Sending ${event} webhook for project: ${projectName}`);

  try {
    const response = await axios.post(WEBHOOK_URL, payload);
    console.log('Response:', response.data);
    return response.data;
  } catch (error) {
    console.error('Error:', error.response?.data || error.message);
    throw error;
  }
}

// Run the test scenario
async function runTest() {
  console.log('Starting webhook test');
  console.log('-------------------');

  try {
    // Start a timer
    console.log('\n1. Starting timer for test project');
    const startResponse = await sendWebhook('timer_start', 'Test Project');
    console.log(`Project ID: ${startResponse.project.id}`);
    console.log(`Time Entry ID: ${startResponse.timeEntry.id}`);

    // Wait for 5 seconds
    console.log('\nWaiting for 5 seconds...');
    await new Promise(resolve => setTimeout(resolve, 5000));

    // Stop the timer
    console.log('\n2. Stopping timer for test project');
    const stopResponse = await sendWebhook('timer_stop', 'Test Project');
    console.log(`Duration: ${stopResponse.timeEntry.duration} seconds`);

    // Rename the project and start another timer
    console.log('\n3. Starting timer with renamed project');
    const renamedResponse = await sendWebhook('timer_start', 'Test Project Renamed');

    // Wait for 3 seconds
    console.log('\nWaiting for 3 seconds...');
    await new Promise(resolve => setTimeout(resolve, 3000));

    // Stop the second timer
    console.log('\n4. Stopping timer for renamed project');
    const stopRenamedResponse = await sendWebhook('timer_stop', 'Test Project Renamed');

    console.log('\nTest completed successfully!');
    console.log('----------------------------');
    console.log('You should now see:');
    console.log('1. A project called "Test Project Renamed" with color #FF5733');
    console.log('2. Two time entries for this project');
    console.log(`3. Visit ${BASE_URL}/projects to see your projects`);
    console.log(`4. Visit ${BASE_URL}/entries to see your time entries`);

  } catch (error) {
    console.log('\nTest failed!');
  }
}

// Run the test
runTest(); 