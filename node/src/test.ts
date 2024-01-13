import { parseXmlConfigFile } from "./config/parse.js";

const config = await parseXmlConfigFile("casparcg.config");
console.log("read", config);
