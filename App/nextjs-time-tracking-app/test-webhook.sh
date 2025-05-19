#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Define the API URL - use focus-dial-app.local or localhost
API_URL="http://localhost:3000/api/webhook"

echo -e "\n${YELLOW}Testing Focus Dial Webhook API${NC}\n"

# Test 1: Simple GET request
echo -e "${YELLOW}Test 1: GET request to check if the webhook is ready${NC}"
curl -s $API_URL | jq
echo

# Test 2: Start a timer for a project
echo -e "${YELLOW}Test 2: POST request to start a timer${NC}"
curl -s -X POST \
  -H "Content-Type: application/json" \
  -d '{
    "action": "start_timer",
    "project_id": 1,
    "project_name": "Test Project",
    "timestamp": '$(date +%s)'
  }' \
  $API_URL | jq
echo

# Sleep for a second to simulate some work
sleep 3

# Test 3: Stop the timer for the same project
echo -e "${YELLOW}Test 3: POST request to stop the timer${NC}"
curl -s -X POST \
  -H "Content-Type: application/json" \
  -d '{
    "action": "stop_timer",
    "project_id": 1,
    "timestamp": '$(date +%s)'
  }' \
  $API_URL | jq
echo

echo -e "\n${GREEN}Testing completed!${NC}\n"

# Now you can update your Focus Dial device to use:
# http://focus-dial-app.local:3000/api/webhook

echo -e "${YELLOW}Update your Focus Dial device to use:${NC}"
echo -e "${GREEN}http://focus-dial-app.local:3000/api/webhook${NC}\n" 