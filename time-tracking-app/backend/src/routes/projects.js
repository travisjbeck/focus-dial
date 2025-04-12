const express = require('express');
const router = express.Router();
const { Project, TimeEntry } = require('../models');

/**
 * GET /api/projects
 * Get all projects
 */
router.get('/', async (req, res) => {
  try {
    const projects = await Project.findAll({
      order: [['name', 'ASC']]
    });
    res.status(200).json(projects);
  } catch (error) {
    console.error('Error fetching projects:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * GET /api/projects/:id
 * Get a specific project by ID
 */
router.get('/:id', async (req, res) => {
  try {
    const project = await Project.findByPk(req.params.id);
    if (!project) {
      return res.status(404).json({ error: 'Project not found' });
    }
    res.status(200).json(project);
  } catch (error) {
    console.error('Error fetching project:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * POST /api/projects
 * Create a new project
 */
router.post('/', async (req, res) => {
  try {
    const { name, color } = req.body;

    // Validate required fields
    if (!name || !color) {
      return res.status(400).json({ error: 'Name and color are required' });
    }

    // Check if project with the same name already exists
    const existingProject = await Project.findOne({ where: { name } });
    if (existingProject) {
      return res.status(409).json({ error: 'A project with this name already exists' });
    }

    // Create the project
    const newProject = await Project.create({
      name,
      color
    });

    res.status(201).json(newProject);
  } catch (error) {
    console.error('Error creating project:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * PUT /api/projects/:id
 * Update a project
 */
router.put('/:id', async (req, res) => {
  try {
    const { name, color } = req.body;
    const projectId = req.params.id;

    // Find the project
    const project = await Project.findByPk(projectId);
    if (!project) {
      return res.status(404).json({ error: 'Project not found' });
    }

    // Check if new name already exists (if name is being changed)
    if (name && name !== project.name) {
      const existingProject = await Project.findOne({ where: { name } });
      if (existingProject) {
        return res.status(409).json({ error: 'A project with this name already exists' });
      }
    }

    // Update the project
    if (name) project.name = name;
    if (color) project.color = color;
    await project.save();

    res.status(200).json(project);
  } catch (error) {
    console.error('Error updating project:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * DELETE /api/projects/:id
 * Delete a project
 */
router.delete('/:id', async (req, res) => {
  try {
    const projectId = req.params.id;

    // Find the project
    const project = await Project.findByPk(projectId);
    if (!project) {
      return res.status(404).json({ error: 'Project not found' });
    }

    // Check if project has associated time entries
    const timeEntriesCount = await TimeEntry.count({ where: { project_id: projectId } });
    if (timeEntriesCount > 0) {
      return res.status(409).json({
        error: 'Cannot delete project with associated time entries',
        timeEntriesCount
      });
    }

    // Delete the project
    await project.destroy();

    res.status(200).json({ message: 'Project deleted successfully' });
  } catch (error) {
    console.error('Error deleting project:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router; 