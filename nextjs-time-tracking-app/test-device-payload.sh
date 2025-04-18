#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Define the API URL - use localhost for testing
API_URL="http://localhost:3000/api/webhook"

echo -e "\n${YELLOW}Testing Focus Dial Device Webhook Payload Issue${NC}\n"

# This is the actual payload from the device (with nested action JSON string)
echo -e "${YELLOW}Test 1: Sending the exact device payload with nested action JSON${NC}"
curl -s -X POST \
  -H "Content-Type: application/json" \
  -d '{"action":"{\"action\":\"stop\",\"device_project_id\":\"E86BEA3003D4-5\",\"project_name\":\"Deal Probe\",\"project_color\":\"#4900f5\"}"}' \
  $API_URL | jq
echo

# Now let's try the correct format
echo -e "${YELLOW}Test 2: Sending the correctly formatted payload${NC}"
curl -s -X POST \
  -H "Content-Type: application/json" \
  -d '{
    "action": "stop_timer",
    "device_project_id": "E86BEA3003D4-5",
    "project_name": "Deal Probe",
    "project_color": "#4900f5",
    "timestamp": '$(date +%s)'
  }' \
  $API_URL | jq
echo

# Let's try a modified webhook handler that can parse the nested action
echo -e "${YELLOW}Test 3: Try to fix the webhook in the server side - Modified endpoint${NC}"
curl -s -X POST \
  -H "Content-Type: application/json" \
  -d '{"action":"{\"action\":\"stop\",\"device_project_id\":\"E86BEA3003D4-5\",\"project_name\":\"Deal Probe\",\"project_color\":\"#4900f5\"}"}' \
  "$API_URL?debug=true" | jq
echo

echo -e "\n${GREEN}Testing completed. Check the responses to understand the issue.${NC}\n" 