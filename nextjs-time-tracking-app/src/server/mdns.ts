import mdns from 'multicast-dns';
import os from 'os';

/**
 * Configure mDNS advertisement for the Focus Dial App
 * This allows the hardware device to discover the app using focus-dial-app.local
 */
export function setupMDNS() {
  const hostname = 'focus-dial-app';
  const fullHostname = `${hostname}.local`;

  // Get the local IP address
  const networkInterfaces = os.networkInterfaces();
  let localIP = '';

  // Find the first non-internal IPv4 address
  Object.values(networkInterfaces).forEach((interfaces) => {
    if (interfaces) {
      interfaces.forEach((iface) => {
        if (iface.family === 'IPv4' && !iface.internal) {
          localIP = iface.address;
        }
      });
    }
  });

  if (!localIP) {
    console.error('Could not determine local IP address for mDNS');
    return;
  }

  console.log(`Setting up mDNS advertisement for ${fullHostname} on IP ${localIP}`);

  // Create multicast-dns instance
  const mdnsServer = mdns();

  // Handle queries
  mdnsServer.on('query', (query) => {
    // Respond to queries for our hostname
    const matchingQuestions = query.questions.filter(
      (question) =>
        (question.name === hostname || question.name === fullHostname) &&
        (question.type === 'A')
    );

    if (matchingQuestions.length > 0) {
      console.log(`Responding to mDNS query for ${fullHostname}`);

      mdnsServer.respond({
        answers: [
          {
            name: fullHostname,
            type: 'A',
            ttl: 300,
            data: localIP
          }
        ]
      });
    }
  });

  // Announce service on startup
  function announceService() {
    mdnsServer.respond({
      answers: [
        {
          name: fullHostname,
          type: 'A',
          ttl: 300,
          data: localIP
        }
      ]
    });
  }

  // Announce immediately and then periodically
  announceService();
  const intervalId = setInterval(announceService, 60000); // Announce every minute

  console.log(`mDNS hostname ${fullHostname} configured successfully`);
  return () => {
    clearInterval(intervalId);
    mdnsServer.destroy();
  };
} 