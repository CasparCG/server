import { createInterface } from "node:readline";
import { Native } from "./native.js";
import { AMCPClientBatchInfo, AMCPProtocolStrategy } from "./amcp/protocol.js";
import { ChannelLocks } from "./amcp/channel_locks.js";
import { Client } from "./amcp/client.js";
import { AMCPServer } from "./amcp/server.js";
import { parseXmlConfigFile } from "./config/parse.js";

const rl = createInterface({
    input: process.stdin,
    output: process.stdout,
});

const config = await parseXmlConfigFile("casparcg.config");

console.log(Native);
Native.init(config);

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
        if (line.toUpperCase() === "BYE") {
            rl.close();
            return;
        }

        protocol
            .parse(consoleClient, line.trim())
            .then((answer) => {
                const str = answer.join("\n");
                console.log(str);
            })
            .catch((e) => {
                console.error(e);
            });
    } catch (e) {
        console.error(e);
    }

    rl.prompt();
}).on("close", function () {
    Native.stop();

    console.log("Successfully shutdown CasparCG Server.");

    process.exit(0);
});
