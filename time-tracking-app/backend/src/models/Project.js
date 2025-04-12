const { DataTypes } = require('sequelize');
const { sequelize } = require('../config/database');

const Project = sequelize.define('Project', {
  id: {
    type: DataTypes.INTEGER,
    primaryKey: true,
    autoIncrement: true
  },
  name: {
    type: DataTypes.STRING,
    allowNull: false,
    unique: true
  },
  color: {
    type: DataTypes.STRING,
    allowNull: false,
    validate: {
      is: /^#[0-9A-F]{6}$/i // Hex color validation
    }
  }
}, {
  // Enable timestamps (created_at, updated_at)
  timestamps: true,
  // Use snake_case instead of camelCase for attributes
  underscored: true
});

module.exports = Project; 