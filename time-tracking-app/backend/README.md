# Focus Dial - Time Tracking Backend API

This is the backend API for the Focus Dial time tracking application. It receives webhooks from the Focus Dial hardware device, stores time entries in a SQLite database, and provides REST API endpoints for the frontend application.

## Prerequisites

- Node.js 16.x or higher
- npm or yarn
- SQLite3

## Installation

1. Clone the repository
2. Navigate to the backend directory:
   ```
   cd time-tracking-app/backend
   ```
3. Install dependencies:
   ```
   npm install
   # or
   yarn install
   ```
4. Create the data directory if it doesn't exist:
   ```
   mkdir -p data
   ```

## Configuration

The application uses default settings that should work out of the box. The SQLite database will be created at `data/focus_dial.sqlite` when the server first starts.

## Running the Server

### Development

```
npm run dev
# or
yarn dev
```

This will start the server with nodemon for automatic reloading when files change.

### Production

```
npm start
# or
yarn start
```

The server will run on port 3000 by default. You can change this by setting the `PORT` environment variable.

## API Endpoints

### Webhook

- `POST /api/webhook`: Receives webhook data from the Focus Dial

### Projects

- `GET /api/projects`: Get all projects
- `GET /api/projects/:id`: Get a specific project
- `POST /api/projects`: Create a new project
- `PUT /api/projects/:id`: Update a project
- `DELETE /api/projects/:id`: Delete a project

### Time Entries

- `GET /api/time_entries`: Get all time entries with optional filtering
- `GET /api/time_entries/:id`: Get a specific time entry
- `PUT /api/time_entries/:id`: Update a time entry
- `DELETE /api/time_entries/:id`: Delete a time entry

### Invoices

- `GET /api/invoices`: Get all invoices
- `GET /api/invoices/:id`: Get a specific invoice with its items
- `POST /api/invoices`: Create a new invoice for uninvoiced time entries
- `PUT /api/invoices/:id`: Update an invoice status
- `DELETE /api/invoices/:id`: Delete an invoice (only if it's a draft)

## Setting Up on Raspberry Pi

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

4. Start the server with PM2:
   ```
   pm2 start src/index.js --name focus-dial-backend
   ```

5. Configure PM2 to start on boot:
   ```
   pm2 startup
   pm2 save
   ```

6. Setup Avahi for mDNS (allows accessing the app via focus-dial-app.local):
   ```
   sudo apt install -y avahi-daemon
   ```

7. Configure the Focus Dial to send webhooks to the Raspberry Pi:
   - Access the Focus Dial web interface at focus-dial.local
   - Set the webhook URL to `http://<RASPBERRY_PI_IP>:3000/api/webhook`
   or if you've set up Avahi: `http://focus-dial-app.local:3000/api/webhook` 