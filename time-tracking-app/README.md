# Focus Dial - Time Tracking Application

This project is a full-stack web application that complements the Focus Dial hardware device. It receives webhook data from the Focus Dial, stores time entries, and provides a dashboard for viewing and managing time tracking data.

## Project Structure

The application is split into two main parts:

1. **Backend API** - Node.js/Express server with SQLite database
2. **Frontend SPA** - React application with Tailwind CSS

## Setup and Installation

### Prerequisites

- Raspberry Pi (3 or newer) with Raspberry Pi OS
- Node.js 16.x or higher
- npm or yarn
- SQLite3
- Focus Dial hardware device configured to send webhooks

### Backend Setup

See [backend/README.md](./backend/README.md) for detailed backend setup instructions.

### Frontend Setup

See [frontend/README.md](./frontend/README.md) for detailed frontend setup instructions.

## Complete Installation on Raspberry Pi

1. Install Node.js:
   ```
   curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
   sudo apt install -y nodejs
   ```

2. Install SQLite:
   ```
   sudo apt install -y sqlite3
   ```

3. Install PM2 for process management:
   ```
   sudo npm install -g pm2
   ```

4. Clone the repository:
   ```
   git clone <repository-url>
   cd focus-dial/time-tracking-app
   ```

5. Install backend dependencies:
   ```
   cd backend
   npm install
   ```

6. Install frontend dependencies:
   ```
   cd ../frontend
   npm install
   ```

7. Build the frontend:
   ```
   npm run build
   ```

8. Start the backend server with PM2:
   ```
   cd ../backend
   pm2 start src/index.js --name focus-dial-backend
   ```

9. Configure PM2 to start on boot:
   ```
   pm2 startup
   pm2 save
   ```

10. Setup Avahi for mDNS (allows accessing via focus-dial-app.local):
    ```
    sudo apt install -y avahi-daemon
    ```

11. Configure the Focus Dial to send webhooks to the Raspberry Pi:
    - Access the Focus Dial web interface at focus-dial.local
    - Set the webhook URL to `http://<RASPBERRY_PI_IP>:3000/api/webhook`
    or if you've set up Avahi: `http://focus-dial-app.local:3000/api/webhook`

## Accessing the Application

Once installed, you can access the application at:

- `http://<RASPBERRY_PI_IP>:3000`
- or if you've set up Avahi: `http://focus-dial-app.local:3000`

## Features

- Receive and process webhooks from Focus Dial
- Track time entries for different projects
- View time entries with filtering options
- Manage projects
- Generate invoices from time entries

## Development

For local development, you can run both the backend and frontend separately:

1. Start the backend:
   ```
   cd backend
   npm run dev
   ```

2. Start the frontend:
   ```
   cd frontend
   npm run dev
   ```

The frontend will be available at http://localhost:5173 and will proxy API requests to the backend at http://localhost:3000.

## Backup Strategy

The SQLite database is stored at `backend/data/focus_dial.sqlite`. To back up the data, simply copy this file to a secure location periodically. You can also setup an automated backup script using cron.

## License

MIT 