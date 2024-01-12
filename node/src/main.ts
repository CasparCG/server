import { createInterface } from "node:readline";
import { Native } from "./native.js";
import { AMCPClientBatchInfo, AMCPProtocolStrategy } from "./amcp/protocol.js";
import { ChannelLocks } from "./amcp/channel_locks.js";
import { Client } from "./amcp/client.js";
import { AMCPServer } from "./amcp/server.js";

const rl = createInterface({
    input: process.stdin,
    output: process.stdout,
});

console.log(Native);
Native.init();

// setInterval(() => {
//     // Keep the app alive
// }, 10000);

rl.setPrompt("");
rl.prompt();

const locks = new ChannelLocks("");
const protocol = new AMCPProtocolStrategy(locks);
const server = new AMCPServer(5250, protocol, locks);
console.log(server);

const consoleClient: Client = {
    id: "console",
    address: "console",
    batch: new AMCPClientBatchInfo(),
};

rl.on("line", function (line) {
    try {
        const answer = protocol.parse(consoleClient, line.trim());
        console.log(answer);
    } catch (e) {
        console.error(e);
    }

    rl.prompt();
}).on("close", function () {
    Native.stop();

    console.log("Successfully shutdown CasparCG Server.");

    process.exit(0);
});
