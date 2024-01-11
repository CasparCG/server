import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const lib = require("../src/build/Release/casparcg.node");

console.log(lib);
console.log(lib.hello());
