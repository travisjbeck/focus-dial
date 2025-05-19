import { createServer } from 'http';
import { parse } from 'url';
import next from 'next';
import { setupMDNS } from './server/mdns';

const dev = process.env.NODE_ENV !== 'production';
const hostname = 'localhost';
const port = process.env.PORT ? parseInt(process.env.PORT, 10) : 3000;

// Initialize the Next.js app
const app = next({ dev, hostname, port });
const handle = app.getRequestHandler();

// Prepare the Next.js app
app.prepare().then(() => {
  // Create HTTP server
  const server = createServer(async (req, res) => {
    try {
      // Parse request URL
      const parsedUrl = parse(req.url!, true);

      // Let Next.js handle the request
      await handle(req, res, parsedUrl);
    } catch (err) {
      console.error('Error occurred handling request:', err);
      res.statusCode = 500;
      res.end('Internal Server Error');
    }
  });

  // Listen on the specified port
  server.listen(port, () => {
    // Setup mDNS after server starts
    const cleanupMDNS = setupMDNS();

    console.log(`> Ready on http://${hostname}:${port}`);
    console.log(`> Also available at http://focus-dial-app.local:${port}`);

    // Handle graceful shutdown
    const shutdown = () => {
      console.log('Shutting down server...');
      if (cleanupMDNS) cleanupMDNS();
      server.close(() => {
        console.log('Server closed');
        process.exit(0);
      });
    };

    // Handle termination signals
    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);
  });
}); 