const { Sequelize } = require('sequelize');
const path = require('path');

// Define the path to the SQLite database file
const dbPath = path.resolve(__dirname, '../../data/focus_dial.sqlite');

// Initialize Sequelize with SQLite
const sequelize = new Sequelize({
  dialect: 'sqlite',
  storage: dbPath,
  logging: false // Set to console.log for SQL query logging during development
});

// Test the database connection
const testConnection = async () => {
  try {
    await sequelize.authenticate();
    console.log('Database connection has been established successfully.');
  } catch (error) {
    console.error('Unable to connect to the database:', error);
  }
};

module.exports = {
  sequelize,
  testConnection
}; 