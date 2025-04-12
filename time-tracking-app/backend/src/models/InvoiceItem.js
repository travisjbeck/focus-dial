const { DataTypes } = require('sequelize');
const { sequelize } = require('../config/database');
const Invoice = require('./Invoice');
const TimeEntry = require('./TimeEntry');

const InvoiceItem = sequelize.define('InvoiceItem', {
  id: {
    type: DataTypes.INTEGER,
    primaryKey: true,
    autoIncrement: true
  }
}, {
  // Enable timestamps (created_at, updated_at)
  timestamps: true,
  // Use snake_case instead of camelCase for attributes
  underscored: true
});

// Define relationships
InvoiceItem.belongsTo(Invoice, {
  foreignKey: {
    name: 'invoice_id',
    allowNull: false
  },
  onDelete: 'CASCADE'
});

InvoiceItem.belongsTo(TimeEntry, {
  foreignKey: {
    name: 'time_entry_id',
    allowNull: false,
    unique: true
  },
  onDelete: 'CASCADE'
});

Invoice.hasMany(InvoiceItem, {
  foreignKey: 'invoice_id'
});

TimeEntry.hasOne(InvoiceItem, {
  foreignKey: 'time_entry_id'
});

module.exports = InvoiceItem; 