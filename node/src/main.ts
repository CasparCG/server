import { createInterface } from "node:readline";
import { Native } from "./native.js";
import { tokenize } from "./tokenize.js";
import { AMCPCommand } from "./amcp.js";

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

rl.on("line", function (line) {
    try {
        const tokens = tokenize(line.trim());
        const cmd: AMCPCommand = new Native.AMCPCommand(tokens);
        console.log(cmd);

        Native.executeCommandBatch([cmd]);
    } catch (e) {
        console.error(e);
    }

    rl.prompt();
}).on("close", function () {
    Native.stop();

    console.log("Successfully shutdown CasparCG Server.");

    process.exit(0);
});
