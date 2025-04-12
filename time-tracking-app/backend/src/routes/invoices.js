const express = require('express');
const router = express.Router();
const { Invoice, InvoiceItem, TimeEntry, Project } = require('../models');
const { sequelize } = require('../config/database');

/**
 * GET /api/invoices
 * Get all invoices
 */
router.get('/', async (req, res) => {
  try {
    const invoices = await Invoice.findAll({
      include: [
        { model: Project, attributes: ['id', 'name', 'color'] }
      ],
      order: [['invoice_date', 'DESC']]
    });

    res.status(200).json(invoices);
  } catch (error) {
    console.error('Error fetching invoices:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * GET /api/invoices/:id
 * Get a specific invoice with its items
 */
router.get('/:id', async (req, res) => {
  try {
    const invoice = await Invoice.findByPk(req.params.id, {
      include: [
        { model: Project, attributes: ['id', 'name', 'color'] },
        {
          model: InvoiceItem,
          include: [{
            model: TimeEntry,
            attributes: ['id', 'start_time', 'end_time', 'duration_seconds', 'description']
          }]
        }
      ]
    });

    if (!invoice) {
      return res.status(404).json({ error: 'Invoice not found' });
    }

    res.status(200).json(invoice);
  } catch (error) {
    console.error('Error fetching invoice:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * POST /api/invoices
 * Create a new invoice for uninvoiced time entries
 */
router.post('/', async (req, res) => {
  // Use a transaction to ensure all or nothing
  const transaction = await sequelize.transaction();

  try {
    const { project_id, time_entry_ids, total_amount } = req.body;

    // Validate required fields
    if (!project_id || !time_entry_ids || !Array.isArray(time_entry_ids) || time_entry_ids.length === 0) {
      await transaction.rollback();
      return res.status(400).json({
        error: 'Project ID and at least one time entry ID are required'
      });
    }

    // Validate project exists
    const project = await Project.findByPk(project_id);
    if (!project) {
      await transaction.rollback();
      return res.status(404).json({ error: 'Project not found' });
    }

    // Validate time entries exist and belong to the specified project
    const timeEntries = await TimeEntry.findAll({
      where: {
        id: time_entry_ids,
        project_id: project_id,
        invoiced: false
      }
    });

    if (timeEntries.length !== time_entry_ids.length) {
      await transaction.rollback();
      return res.status(400).json({
        error: 'Some time entries were not found, already invoiced, or do not belong to the specified project'
      });
    }

    // Calculate total amount if not provided
    let invoiceTotal = total_amount;
    if (!invoiceTotal) {
      // Default calculation logic - can be customized
      // For now, just sum up the duration_seconds (assuming hourly rate is 1.0)
      invoiceTotal = timeEntries.reduce((sum, entry) => {
        return sum + (entry.duration_seconds || 0);
      }, 0) / 3600; // Convert seconds to hours
    }

    // Create the invoice
    const newInvoice = await Invoice.create({
      project_id,
      invoice_date: new Date(),
      total_amount: invoiceTotal,
      status: 'draft'
    }, { transaction });

    // Create invoice items and mark time entries as invoiced
    for (const entry of timeEntries) {
      await InvoiceItem.create({
        invoice_id: newInvoice.id,
        time_entry_id: entry.id
      }, { transaction });

      // Mark time entry as invoiced
      entry.invoiced = true;
      await entry.save({ transaction });
    }

    // Commit the transaction
    await transaction.commit();

    // Fetch the complete invoice with items
    const completeInvoice = await Invoice.findByPk(newInvoice.id, {
      include: [
        { model: Project, attributes: ['id', 'name', 'color'] },
        {
          model: InvoiceItem,
          include: [{
            model: TimeEntry,
            attributes: ['id', 'start_time', 'end_time', 'duration_seconds', 'description']
          }]
        }
      ]
    });

    res.status(201).json(completeInvoice);
  } catch (error) {
    await transaction.rollback();
    console.error('Error creating invoice:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * PUT /api/invoices/:id
 * Update an invoice status
 */
router.put('/:id', async (req, res) => {
  try {
    const { status } = req.body;
    const invoiceId = req.params.id;

    // Validate status
    if (!status || !['draft', 'sent', 'paid'].includes(status)) {
      return res.status(400).json({
        error: 'Valid status (draft, sent, or paid) is required'
      });
    }

    // Find the invoice
    const invoice = await Invoice.findByPk(invoiceId);
    if (!invoice) {
      return res.status(404).json({ error: 'Invoice not found' });
    }

    // Update the invoice
    invoice.status = status;
    await invoice.save();

    res.status(200).json(invoice);
  } catch (error) {
    console.error('Error updating invoice:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * DELETE /api/invoices/:id
 * Delete an invoice (only if it's a draft)
 */
router.delete('/:id', async (req, res) => {
  // Use a transaction to ensure all or nothing
  const transaction = await sequelize.transaction();

  try {
    const invoiceId = req.params.id;

    // Find the invoice
    const invoice = await Invoice.findByPk(invoiceId, {
      include: [{ model: InvoiceItem }]
    });

    if (!invoice) {
      await transaction.rollback();
      return res.status(404).json({ error: 'Invoice not found' });
    }

    // Only allow deleting draft invoices
    if (invoice.status !== 'draft') {
      await transaction.rollback();
      return res.status(409).json({
        error: 'Only draft invoices can be deleted'
      });
    }

    // Get all time entry IDs related to this invoice
    const timeEntryIds = invoice.InvoiceItems.map(item => item.time_entry_id);

    // Un-mark time entries as invoiced
    await TimeEntry.update(
      { invoiced: false },
      {
        where: { id: timeEntryIds },
        transaction
      }
    );

    // Delete invoice items
    await InvoiceItem.destroy({
      where: { invoice_id: invoiceId },
      transaction
    });

    // Delete the invoice
    await invoice.destroy({ transaction });

    // Commit the transaction
    await transaction.commit();

    res.status(200).json({ message: 'Invoice deleted successfully' });
  } catch (error) {
    await transaction.rollback();
    console.error('Error deleting invoice:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router; 