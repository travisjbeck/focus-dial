const express = require('express');
const cors = require('cors');
const path = require('path');
const { sequelize, testConnection } = require('./config/database');

// Import route modules (will create these next)
const webhookRoutes = require('./routes/webhook');
const projectRoutes = require('./routes/projects');
const timeEntryRoutes = require('./routes/timeEntries');
const invoiceRoutes = require('./routes/invoices');

// Initialize Express app
const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Serve static files from the React app (will be built later)
app.use(express.static(path.join(__dirname, '../../frontend/build')));

// API Routes
app.use('/api/webhook', webhookRoutes);
app.use('/api/projects', projectRoutes);
app.use('/api/time_entries', timeEntryRoutes);
app.use('/api/invoices', invoiceRoutes);

// The "catch-all" route handler for any request that doesn't match the ones above
// This will serve the React app
app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, '../../frontend/build/index.html'));
});

// Sync database and start server
const startServer = async () => {
  // Test database connection
  await testConnection();

  // Sync all models with database
  // Use { force: true } only in development to drop tables and recreate them
  await sequelize.sync({ force: false });
  console.log('Database synchronized');

  // Start server
  app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`API accessible at http://localhost:${PORT}/api`);
    console.log(`Frontend accessible at http://localhost:${PORT}`);
  });
};

// Start the server
startServer().catch(error => {
  console.error('Failed to start server:', error);
}); 