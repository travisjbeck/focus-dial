# thetimer.app OAuth Integration Implementation Guide

## Overview

This document outlines the implementation of OAuth-style authentication between the Timer Device and thetimer.app, allowing seamless connection without manual API key entry.

## Architecture

### Components

1. **Timer Device (Client)**
   - Web interface at `http://thetimer.local`
   - Runs on ESP32-S3 with web server
   - Stores API credentials in NVS

2. **thetimer.app (Server)**
   - Next.js application
   - Handles user authentication
   - Generates device-specific API keys

### Authentication Flow

```
Timer Device                    thetimer.app                    User
     |                              |                             |
     |--- 1. Click "Connect" ------>|                             |
     |                              |                             |
     |<-- 2. Open popup window -----|                             |
     |    (to /device-auth)         |                             |
     |                              |<---- 3. User logs in -------|
     |                              |                             |
     |                              |---- 4. Generate API key --->|
     |                              |                             |
     |<-- 5. postMessage with ------|                             |
     |    credentials               |                             |
     |                              |                             |
     |--- 6. Save credentials ----->|                             |
     |                              |                             |
     |<-- 7. Confirm connection ----|                             |
```

## Implementation Details

### Timer Device Side (Web Interface)

#### 1. Update UI in `firmware/data/index.html`

Replace the current webhook configuration section with:

```html
<section class="card" id="integration-section">
  <div class="card-header">
    <h2>
      <i data-lucide="cloud" class="icon"></i>
      thetimer.app Integration
    </h2>
  </div>
  <div class="form-content">
    <!-- Not Connected State -->
    <div id="not-connected" class="connection-state">
      <div class="integration-info">
        <p>Connect to thetimer.app to:</p>
        <ul>
          <li>Sync your timer data to the cloud</li>
          <li>Access detailed analytics and reports</li>
          <li>Share projects with your team</li>
          <li>7-day free trial, then subscription required</li>
        </ul>
      </div>
      <button type="button" class="btn primary-btn" id="connect-button">
        <i data-lucide="link" class="icon"></i>
        <span>Connect to thetimer.app</span>
      </button>
    </div>
    
    <!-- Connected State -->
    <div id="connected" class="connection-state" style="display: none;">
      <div class="connection-status">
        <div class="status-indicator connected"></div>
        <div class="status-info">
          <p class="status-text">Connected to thetimer.app</p>
          <p class="account-email" id="account-email"></p>
          <p class="last-sync">Last synced: <span id="last-sync-time">Never</span></p>
        </div>
      </div>
      <button type="button" class="btn secondary-btn danger-btn" id="disconnect-button">
        <i data-lucide="unlink" class="icon"></i>
        <span>Disconnect</span>
      </button>
    </div>
  </div>
</section>
```

#### 2. JavaScript OAuth Handler in `firmware/data/app.js`

```javascript
// OAuth Configuration
const TIMER_APP_URL = 'https://thetimer.app';
const DEVICE_AUTH_ENDPOINT = '/device-auth';
const WEBHOOK_ENDPOINT = '/api/webhook';

let authWindow = null;

// Initialize connection state
async function initializeIntegration() {
  const apiKey = await fetchStoredApiKey();
  updateConnectionUI(!!apiKey);
  
  if (apiKey) {
    // Verify connection is still valid
    verifyConnection(apiKey);
  }
}

// Handle Connect button click
document.getElementById('connect-button').addEventListener('click', async () => {
  // Generate state parameter for security
  const state = generateRandomState();
  sessionStorage.setItem('oauth_state', state);
  
  // Get device information
  const deviceInfo = await getDeviceInfo();
  
  // Build OAuth URL
  const authUrl = new URL(DEVICE_AUTH_ENDPOINT, TIMER_APP_URL);
  authUrl.searchParams.append('device_id', deviceInfo.id);
  authUrl.searchParams.append('device_name', deviceInfo.name);
  authUrl.searchParams.append('state', state);
  authUrl.searchParams.append('callback_origin', window.location.origin);
  
  // Open popup window
  const width = 600;
  const height = 700;
  const left = (window.screen.width - width) / 2;
  const top = (window.screen.height - height) / 2;
  
  authWindow = window.open(
    authUrl.toString(),
    'TimerAppAuth',
    `width=${width},height=${height},left=${left},top=${top},toolbar=no,menubar=no`
  );
  
  // Start listening for response
  window.addEventListener('message', handleAuthMessage);
});

// Handle postMessage from thetimer.app
async function handleAuthMessage(event) {
  // Verify origin
  if (event.origin !== TIMER_APP_URL) return;
  
  // Verify state
  const savedState = sessionStorage.getItem('oauth_state');
  if (!savedState || event.data.state !== savedState) {
    console.error('Invalid state parameter');
    return;
  }
  
  // Extract credentials
  const { api_key, account_email, webhook_url } = event.data;
  
  if (api_key) {
    // Save credentials
    await saveCredentials({
      api_key,
      webhook_url: webhook_url || `${TIMER_APP_URL}${WEBHOOK_ENDPOINT}`,
      account_email
    });
    
    // Update UI
    updateConnectionUI(true, account_email);
    
    // Close popup
    if (authWindow) {
      authWindow.close();
    }
    
    // Clean up
    sessionStorage.removeItem('oauth_state');
    window.removeEventListener('message', handleAuthMessage);
    
    showMessage('Successfully connected to thetimer.app!', 'success');
  }
}

// Save credentials to device
async function saveCredentials(credentials) {
  const response = await fetch('/api/webhook', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      webhook_url: credentials.webhook_url,
      api_key: credentials.api_key
    })
  });
  
  if (!response.ok) {
    throw new Error('Failed to save credentials');
  }
  
  // Store additional metadata locally
  localStorage.setItem('timer_app_email', credentials.account_email);
  localStorage.setItem('timer_app_connected_at', new Date().toISOString());
}

// Handle disconnect
document.getElementById('disconnect-button').addEventListener('click', async () => {
  if (confirm('Are you sure you want to disconnect from thetimer.app?')) {
    await fetch('/api/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        webhook_url: '',
        api_key: ''
      })
    });
    
    localStorage.removeItem('timer_app_email');
    localStorage.removeItem('timer_app_connected_at');
    
    updateConnectionUI(false);
    showMessage('Disconnected from thetimer.app', 'success');
  }
});

// Utility functions
function generateRandomState() {
  return Array.from(crypto.getRandomValues(new Uint8Array(16)))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
}

async function getDeviceInfo() {
  // Get device MAC address as unique ID
  const response = await fetch('/api/system/info');
  const info = await response.json();
  
  return {
    id: info.mac_address || 'unknown',
    name: info.hostname || 'Timer Device'
  };
}

function updateConnectionUI(isConnected, email = null) {
  const notConnected = document.getElementById('not-connected');
  const connected = document.getElementById('connected');
  
  if (isConnected) {
    notConnected.style.display = 'none';
    connected.style.display = 'block';
    
    if (email) {
      document.getElementById('account-email').textContent = email;
    } else {
      const savedEmail = localStorage.getItem('timer_app_email');
      if (savedEmail) {
        document.getElementById('account-email').textContent = savedEmail;
      }
    }
    
    // Update last sync time
    updateLastSyncTime();
  } else {
    notConnected.style.display = 'block';
    connected.style.display = 'none';
  }
}
```

### thetimer.app Side (Next.js)

#### 1. Device Authentication Page (`pages/device-auth.tsx`)

```typescript
import { useEffect, useState } from 'react';
import { useRouter } from 'next/router';
import { useSession } from 'next-auth/react';

export default function DeviceAuth() {
  const router = useRouter();
  const { data: session, status } = useSession();
  const [isProcessing, setIsProcessing] = useState(false);
  
  const { device_id, device_name, state, callback_origin } = router.query;

  useEffect(() => {
    // Redirect to login if not authenticated
    if (status === 'unauthenticated') {
      router.push(`/auth/signin?callbackUrl=${encodeURIComponent(router.asPath)}`);
    }
  }, [status, router]);

  useEffect(() => {
    // Process authentication once logged in
    if (session && device_id && state && callback_origin && !isProcessing) {
      handleDeviceAuth();
    }
  }, [session, device_id, state, callback_origin]);

  const handleDeviceAuth = async () => {
    setIsProcessing(true);
    
    try {
      // Create or retrieve API key for this device
      const response = await fetch('/api/devices/authenticate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          device_id,
          device_name,
          user_id: session.user.id
        })
      });
      
      const data = await response.json();
      
      if (data.api_key) {
        // Send credentials back to device
        window.opener.postMessage({
          api_key: data.api_key,
          webhook_url: `${process.env.NEXT_PUBLIC_APP_URL}/api/webhook`,
          account_email: session.user.email,
          state
        }, callback_origin as string);
        
        // Show success and close window
        setTimeout(() => {
          window.close();
        }, 1000);
      }
    } catch (error) {
      console.error('Device authentication failed:', error);
      // Show error to user
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center">
      <div className="max-w-md w-full space-y-8 p-8">
        <div className="text-center">
          <h2 className="text-3xl font-bold">Connect Your Timer Device</h2>
          {session ? (
            <>
              <p className="mt-2 text-gray-600">
                Connecting device to your account: {session.user.email}
              </p>
              <div className="mt-4">
                <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-gray-900 mx-auto"></div>
              </div>
            </>
          ) : (
            <p className="mt-2 text-gray-600">
              Please log in to connect your device
            </p>
          )}
        </div>
      </div>
    </div>
  );
}
```

#### 2. API Endpoint (`pages/api/devices/authenticate.ts`)

```typescript
import { NextApiRequest, NextApiResponse } from 'next';
import { getSession } from 'next-auth/react';
import { prisma } from '@/lib/prisma';
import { generateApiKey } from '@/lib/auth';

export default async function handler(req: NextApiRequest, res: NextApiResponse) {
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  const session = await getSession({ req });
  if (!session) {
    return res.status(401).json({ error: 'Unauthorized' });
  }

  const { device_id, device_name } = req.body;

  try {
    // Check if device already exists
    let device = await prisma.device.findUnique({
      where: { device_id }
    });

    if (device && device.user_id !== session.user.id) {
      return res.status(403).json({ error: 'Device registered to another user' });
    }

    // Generate new API key
    const api_key = generateApiKey();

    if (device) {
      // Update existing device
      device = await prisma.device.update({
        where: { device_id },
        data: {
          api_key,
          last_connected: new Date()
        }
      });
    } else {
      // Create new device
      device = await prisma.device.create({
        data: {
          device_id,
          device_name,
          api_key,
          user_id: session.user.id,
          trial_ends_at: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000) // 7 days
        }
      });
    }

    return res.status(200).json({
      api_key,
      device_id: device.device_id
    });
  } catch (error) {
    console.error('Device authentication error:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
}
```

## Security Considerations

1. **State Parameter**: Prevents CSRF attacks
2. **Origin Verification**: Only accept postMessage from thetimer.app
3. **HTTPS Required**: All communication over secure channels
4. **API Key Rotation**: Implement key rotation mechanism
5. **Device Binding**: API keys tied to specific device IDs

## Implementation Checklist

### Timer Device
- [ ] Update HTML with new integration UI
- [ ] Implement OAuth JavaScript handler
- [ ] Add connection state management
- [ ] Update CSS for connection status indicators
- [ ] Test popup window communication
- [ ] Handle error states gracefully

### thetimer.app
- [ ] Create device authentication page
- [ ] Implement device registration API
- [ ] Add device management to user dashboard
- [ ] Set up trial/subscription validation
- [ ] Configure CORS for device communication
- [ ] Test postMessage flow

## Testing

1. **Happy Path**
   - User clicks connect
   - Logs in successfully
   - Device receives credentials
   - Connection status updates

2. **Error Cases**
   - User closes popup without authenticating
   - Network failure during authentication
   - Invalid credentials
   - Expired trial

3. **Security**
   - Verify state parameter validation
   - Test origin verification
   - Ensure API keys are stored securely

## Future Enhancements

1. **Device Management Dashboard**: Allow users to see all connected devices
2. **Automatic Reconnection**: Handle token refresh
3. **Offline Queue**: Store timer data locally when disconnected
4. **Multi-device Sync**: Sync projects across devices
5. **Push Notifications**: Real-time updates to devices