import { createInterface } from "node:readline";
import { Native } from "./native.js";
import { AMCPClientBatchInfo, AMCPProtocolStrategy } from "./amcp.js";

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

const batch = new AMCPClientBatchInfo(); // Future: there should be one per connected client
const protocol = new AMCPProtocolStrategy();

rl.on("line", function (line) {
    try {
        const answer = protocol.parse(line.trim(), batch);
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
