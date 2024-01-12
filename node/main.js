import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const lib = require("../src/build/Release/casparcg.node");

console.log(lib);
lib.init();

lib.parseCommand("INFO\r\n");

setInterval(() => {
    // Keep the app alive
}, 10000);
