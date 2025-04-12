const { DataTypes } = require('sequelize');
const { sequelize } = require('../config/database');
const Project = require('./Project');

const Invoice = sequelize.define('Invoice', {
  id: {
    type: DataTypes.INTEGER,
    primaryKey: true,
    autoIncrement: true
  },
  invoice_date: {
    type: DataTypes.DATE,
    allowNull: false,
    defaultValue: DataTypes.NOW
  },
  total_amount: {
    type: DataTypes.DECIMAL(10, 2),
    allowNull: false
  },
  status: {
    type: DataTypes.ENUM('draft', 'sent', 'paid'),
    allowNull: false,
    defaultValue: 'draft'
  }
}, {
  // Enable timestamps (created_at, updated_at)
  timestamps: true,
  // Use snake_case instead of camelCase for attributes
  underscored: true
});

// Define relationship with Project
Invoice.belongsTo(Project, {
  foreignKey: {
    name: 'project_id',
    allowNull: false
  },
  onDelete: 'CASCADE'
});

Project.hasMany(Invoice, {
  foreignKey: 'project_id'
});

module.exports = Invoice; 