const express = require('express');
const cors = require('cors');
const path = require('path');
const { sequelize, testConnection } = require('./config/database');

// Import route modules
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
// Add text/plain parsing middleware
app.use(express.text());

// API Routes
app.use('/api/webhook', webhookRoutes);
app.use('/api/projects', projectRoutes);
app.use('/api/time_entries', timeEntryRoutes);
app.use('/api/invoices', invoiceRoutes);

// Sync database and start server
const startServer = async () => {
  // Test database connection
  await testConnection();

  // Sync all models with database
  // Use { force: false } (default) to avoid data loss
  await sequelize.sync({ force: false });
  console.log('Database synchronized');

  // Start server
  app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`API accessible at http://localhost:${PORT}/api`);
  });
};

// Start the server
startServer().catch(error => {
  console.error('Failed to start server:', error);
}); 