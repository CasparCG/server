import { createRequire } from "node:module";
import { createInterface } from "node:readline";

const require = createRequire(import.meta.url);
const lib = require("../src/build/Release/casparcg.node");

const rl = createInterface({
    input: process.stdin,
    output: process.stdout,
});

console.log(lib);
lib.init();

// setInterval(() => {
//     // Keep the app alive
// }, 10000);

rl.setPrompt("");
rl.prompt();

rl.on("line", function (line) {
    lib.parseCommand(line.trim() + "\r\n");

    rl.prompt();
}).on("close", function () {
    lib.stop();

    console.log("Successfully shutdown CasparCG Server.");

    process.exit(0);
});
