const addon = require('./build/Release/addon.node');

async function main() {
    console.log(`[Node.js Parent ${process.pid}] Starting main Node.js process.`);

    try {
        const result = await addon.forkAndRun(123);
        console.log(`[Node.js Parent ${process.pid}] Received result from C++ fork: ${result}`);

        const result2 = await addon.forkAndRun(45);
        console.log(`[Node.js Parent ${process.pid}] Received second result from C++ fork: ${result2}`);

    } catch (err) {
        console.error(`[Node.js Parent ${process.pid}] Error calling C++ fork:`, err.message);
    }

    console.log(`[Node.js Parent ${process.pid}] Node.js process continues...`);
    // Keep the event loop alive for a bit to show it's not blocked permanently
    setTimeout(() => {
        console.log(`[Node.js Parent ${process.pid}] Node.js process finishing.`);
    }, 1000);
}

main();
