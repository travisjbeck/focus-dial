const express = require('express');
const router = express.Router();
const { TimeEntry, Project } = require('../models');
const { Op } = require('sequelize');

/**
 * GET /api/time_entries
 * Get all time entries with optional filtering
 * Query params:
 * - project_id: Filter by project
 * - start_date: Filter entries with start_time >= start_date
 * - end_date: Filter entries with start_time <= end_date
 * - invoiced: Filter by invoiced status (true/false)
 */
router.get('/', async (req, res) => {
  try {
    const { project_id, start_date, end_date, invoiced } = req.query;

    // Build the where clause based on query parameters
    const whereClause = {};

    if (project_id) {
      whereClause.project_id = project_id;
    }

    // Date range filtering
    if (start_date || end_date) {
      whereClause.start_time = {};

      if (start_date) {
        whereClause.start_time[Op.gte] = new Date(start_date);
      }

      if (end_date) {
        whereClause.start_time[Op.lte] = new Date(end_date);
      }
    }

    // Invoiced status filtering
    if (invoiced !== undefined) {
      whereClause.invoiced = invoiced === 'true';
    }

    const timeEntries = await TimeEntry.findAll({
      where: whereClause,
      include: [{ model: Project, attributes: ['id', 'name', 'color'] }],
      order: [['start_time', 'DESC']]
    });

    res.status(200).json(timeEntries);
  } catch (error) {
    console.error('Error fetching time entries:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * GET /api/time_entries/:id
 * Get a specific time entry by ID
 */
router.get('/:id', async (req, res) => {
  try {
    const timeEntry = await TimeEntry.findByPk(req.params.id, {
      include: [{ model: Project, attributes: ['id', 'name', 'color'] }]
    });

    if (!timeEntry) {
      return res.status(404).json({ error: 'Time entry not found' });
    }

    res.status(200).json(timeEntry);
  } catch (error) {
    console.error('Error fetching time entry:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * PUT /api/time_entries/:id
 * Update a time entry
 */
router.put('/:id', async (req, res) => {
  try {
    const { description, project_id, invoiced, start_time, end_time } = req.body;
    const timeEntryId = req.params.id;

    // Find the time entry
    const timeEntry = await TimeEntry.findByPk(timeEntryId);
    if (!timeEntry) {
      return res.status(404).json({ error: 'Time entry not found' });
    }

    // Validate project_id if provided
    if (project_id) {
      const projectExists = await Project.findByPk(project_id);
      if (!projectExists) {
        return res.status(400).json({ error: 'Project not found' });
      }
    }

    // Update fields if provided
    if (description !== undefined) timeEntry.description = description;
    if (project_id !== undefined) timeEntry.project_id = project_id;
    if (invoiced !== undefined) timeEntry.invoiced = invoiced;

    // Handle time updates
    let recalculateDuration = false;

    if (start_time !== undefined) {
      timeEntry.start_time = new Date(start_time);
      recalculateDuration = true;
    }

    if (end_time !== undefined) {
      timeEntry.end_time = end_time ? new Date(end_time) : null;
      recalculateDuration = true;
    }

    // Recalculate duration if needed
    if (recalculateDuration && timeEntry.start_time && timeEntry.end_time) {
      timeEntry.duration_seconds = Math.round(
        (timeEntry.end_time - timeEntry.start_time) / 1000
      );
    }

    await timeEntry.save();

    // Fetch the updated entry with project details
    const updatedTimeEntry = await TimeEntry.findByPk(timeEntryId, {
      include: [{ model: Project, attributes: ['id', 'name', 'color'] }]
    });

    res.status(200).json(updatedTimeEntry);
  } catch (error) {
    console.error('Error updating time entry:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * DELETE /api/time_entries/:id
 * Delete a time entry
 */
router.delete('/:id', async (req, res) => {
  try {
    const timeEntryId = req.params.id;

    // Find the time entry
    const timeEntry = await TimeEntry.findByPk(timeEntryId);
    if (!timeEntry) {
      return res.status(404).json({ error: 'Time entry not found' });
    }

    // Check if the time entry is invoiced
    if (timeEntry.invoiced) {
      return res.status(409).json({
        error: 'Cannot delete an invoiced time entry'
      });
    }

    // Delete the time entry
    await timeEntry.destroy();

    res.status(200).json({ message: 'Time entry deleted successfully' });
  } catch (error) {
    console.error('Error deleting time entry:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router; 