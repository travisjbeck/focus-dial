/**
 * Webhook Testing Script
 * This script simulates webhook requests from the Focus Dial device
 * to test the webhook handler implementation.
 */

const axios = require('axios');

// Configuration
const WEBHOOK_URL = 'http://localhost:3000/api/webhook'; // Adjust if your server runs on a different port

// Test scenarios
const testCases = [
  {
    name: 'Test Case 1: Create new project with timer start',
    payload: {
      event: 'timer_start',
      device_project_id: 'test-device-123',
      project_name: 'Test Project 1',
      project_color: '#FF5733'
    }
  },
  {
    name: 'Test Case 2: Update existing project with timer start',
    payload: {
      event: 'timer_start',
      device_project_id: 'test-device-123', // Same ID as Test Case 1
      project_name: 'Test Project 1 (Renamed)', // Name changed
      project_color: '#33FF57' // Color changed
    }
  },
  {
    name: 'Test Case 3: Stop timer',
    payload: {
      event: 'timer_stop',
      device_project_id: 'test-device-123'
    }
  },
  {
    name: 'Test Case 4: Create another project with timer start',
    payload: {
      event: 'timer_start',
      device_project_id: 'test-device-456', // Different ID
      project_name: 'Test Project 2',
      project_color: '#3357FF'
    }
  },
  {
    name: 'Test Case 5: Test missing device_project_id error',
    payload: {
      event: 'timer_start',
      project_name: 'Invalid Project',
      project_color: '#FF0000'
    }
  }
];

// Run tests sequentially
async function runTests() {
  console.log('Starting webhook test suite...\n');

  for (const [index, testCase] of testCases.entries()) {
    console.log(`Running ${testCase.name}...`);

    try {
      const response = await axios.post(WEBHOOK_URL, testCase.payload);
      console.log(`Status: ${response.status}`);
      console.log(`Response: ${JSON.stringify(response.data)}`);
    } catch (error) {
      if (error.response) {
        // The server responded with a status code outside the 2xx range
        console.log(`Status: ${error.response.status}`);
        console.log(`Response: ${JSON.stringify(error.response.data)}`);
      } else {
        console.error(`Error: ${error.message}`);
      }
    }

    console.log('\n----------------------------\n');

    // Add a small delay between requests
    if (index < testCases.length - 1) {
      await new Promise(resolve => setTimeout(resolve, 1000));
    }
  }

  console.log('Test suite completed.');
}

// Make sure the server is running before executing tests
console.log('Please ensure the backend server is running before continuing.');
console.log('Press Enter to start the tests...');

process.stdin.once('data', () => {
  runTests().catch(console.error);
}); 