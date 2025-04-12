const { DataTypes } = require('sequelize');
const { sequelize } = require('../config/database');
const Project = require('./Project');

const TimeEntry = sequelize.define('TimeEntry', {
  id: {
    type: DataTypes.INTEGER,
    primaryKey: true,
    autoIncrement: true
  },
  start_time: {
    type: DataTypes.DATE,
    allowNull: false
  },
  end_time: {
    type: DataTypes.DATE,
    allowNull: true
  },
  duration_seconds: {
    type: DataTypes.INTEGER,
    allowNull: true
  },
  description: {
    type: DataTypes.TEXT,
    allowNull: true
  },
  invoiced: {
    type: DataTypes.BOOLEAN,
    allowNull: false,
    defaultValue: false
  }
}, {
  // Enable timestamps (created_at, updated_at)
  timestamps: true,
  // Use snake_case instead of camelCase for attributes
  underscored: true
});

// Define relationship with Project
TimeEntry.belongsTo(Project, {
  foreignKey: {
    name: 'project_id',
    allowNull: false
  },
  onDelete: 'CASCADE'
});

Project.hasMany(TimeEntry, {
  foreignKey: 'project_id'
});

module.exports = TimeEntry; 