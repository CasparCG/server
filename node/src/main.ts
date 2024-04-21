import { createInterface } from "node:readline";
import { format as formatDateTime } from "date-fns";
import { Native } from "./native.js";
import { AMCPClientBatchInfo, AMCPProtocolStrategy } from "./amcp/protocol.js";
import { ChannelLocks } from "./amcp/channel_locks.js";
import { Client } from "./amcp/client.js";
import { AMCPServer } from "./amcp/server.js";
import { CasparCGConfiguration, VideoMode } from "./config/type.js";
import {
    AMCPCommandContext,
    AMCPCommandRepository,
} from "./amcp/command_repository.js";
import path from "node:path";
import { OscSender } from "./osc.js";

// import { parseXmlConfigFile } from "./config/parse.js";

const rl = createInterface({
    input: process.stdin,
    output: process.stdout,
});

// const config = await parseXmlConfigFile("casparcg.config");
const config: CasparCGConfiguration = {
    logLevel: "info",
    logAlignColumns: true,

    paths: {
        mediaPath: "/home/julus/Projects/caspar_media/",
        logPath: null,
        dataPath: "data/",
        templatePath: "/home/julus/Projects/caspar_templates/",
    },
    lockClearPhrase: "",

    flash: {
        enabled: false,
        bufferDepth: "auto",
        templateHosts: [],
    },
    ffmpeg: {
        producer: {
            autoDeinterlace: "interlaced",
            threads: 4,
        },
    },
    html: {
        remoteDebuggingPort: 0,
        enableGpu: false,
        angleBackend: "gl",
    },
    systemAudio: {
        producer: {
            defaultDeviceName: null,
        },
    },
    ndi: {
        autoLoad: false,
    },

    videoModes: [],

    channels: [
        {
            videoMode: VideoMode._720p5000,
            consumers: [
                {
                    type: "screen",
                    device: 1,
                    "aspect-ratio": "default",
                    stretch: "none",
                    windowed: true,
                    "key-only": false,
                    vsync: false,
                    borderless: false,
                    "always-on-top": false,

                    x: 0,
                    y: 0,
                    width: 0,
                    height: 0,

                    "sbs-key": false,
                    "colour-space": "RGB",
                },
            ],
            producers: {
                10: "AMB LOOP",
                100: "/home/julus/Projects/caspar_media/test",
            },
        },
    ],

    controllers: [],
    osc: {
        defaultPort: 6250,
        disableSendToAMCPClients: false,
        predefinedClients: [],
    },
    mediaScanner: {
        host: "localhost",
        port: 8000,
    },
};

const channelStateStore = new Map<number, Record<string, any[]>>();
const oscSender = new OscSender();

console.log(Native);
Native.init(
    {
        paths: {
            initial: process.cwd(),
            media: config.paths.mediaPath,
            template: config.paths.templatePath,
        },
        moduleConfig: {
            //     ffmpeg: {
            //         producer: {
            //             threads: config.ffmpeg.producer.threads,
            //             "auto-deinterlace": config.ffmpeg.producer.autoDeinterlace,
            //         },
            //     },
            //     flash: {
            //         enabled: config.flash.enabled,
            //         // TODO - fix these
            //         // "buffer-depth": config.flash.bufferDepth,
            //         // templateHosts: config.flash.templateHosts,
            //     },
            //     html: {
            //         "enable-gpu": config.html.enableGpu,
            //         "angle-backend": config.html.angleBackend,
            //         "remote-debugging-port": config.html.remoteDebuggingPort,
            //     },
            //     ndi: {
            //         "auto-load": config.ndi.autoLoad,
            //     },
            //     "system-audio": {
            //         producer: {
            //             "default-device-name":
            //                 config.systemAudio.producer.defaultDeviceName ??
            //                 undefined,
            //         },
            //     },
        },
    },
    (timestamp, message, level) => {
        const timestampStr = formatDateTime(
            timestamp,
            "yyyy-MM-dd HH:mm:ss.SSS"
        );

        console.log("LOG: ", `${timestampStr} ${level} ${message}`);
    },
    (channelId: number, newState: Record<string, any[]>) => {
        newState["_generated"] = [Date.now()];
        channelStateStore.set(channelId, newState);

        oscSender.sendState(channelId, newState);
    }
);

for (const videoMode of config.videoModes) {
    Native.ConfigAddCustomVideoFormat(videoMode);
}
console.log("Initialized video modes.");

function makeAbsolute(myPath: string) {
    if (path.isAbsolute(myPath)) {
        return myPath;
    } else {
        return path.join(process.cwd(), myPath);
    }
}
config.paths.mediaPath = makeAbsolute(config.paths.mediaPath);
if (config.paths.logPath !== null)
    config.paths.logPath = makeAbsolute(config.paths.logPath);
config.paths.dataPath = makeAbsolute(config.paths.dataPath);
config.paths.templatePath = makeAbsolute(config.paths.templatePath);

console.log("config", JSON.stringify(config, undefined, 4));

const context: AMCPCommandContext = {
    configuration: config,
    shutdown: (restart) => {
        Native.stop();
        process.exit(restart ? 5 : 0);
    },
    osc: oscSender,
    channelCount: config.channels.length,
    channelStateStore,
    deferedTransforms: new Map(),
};

const locks = new ChannelLocks(config.lockClearPhrase);
const commandRepository = new AMCPCommandRepository();
const protocol = new AMCPProtocolStrategy(commandRepository, locks, context);

for (const client of config.osc.predefinedClients) {
    if (!oscSender.addOscPredefinedClient(client.address, client.port)) {
        console.log(
            `failed to add osc client: ${client.address}:${client.port}`
        );
    }
}

const startupClient: Client = {
    id: "init",
    address: "init",
    batch: new AMCPClientBatchInfo(),
};

if (config.channels.length === 0) {
    throw new Error("There must be at least one channel defined");
}
for (const channel of config.channels) {
    const channelIndex = Native.AddChannel(channel.videoMode);

    // Consumers
    for (const consumer of channel.consumers) {
        try {
            const port = Native.AddChannelConsumer(channelIndex, consumer);
            console.log(`Added consumer ${channelIndex}-${port}`);
        } catch (e) {
            console.error(`Add consumer failed: `, e);
        }
    }

    // Producers
    for (const [layerStr, amcp] of Object.entries(channel.producers)) {
        if (!amcp) continue;

        const layerIndex = Number(layerStr);
        if (isNaN(layerIndex)) {
            console.log(
                `Invalid producer layer index ${layerStr} for channel ${channelIndex}`
            );
            continue;
        }

        const amcpStr = `PLAY ${channelIndex}-${layerIndex} ${amcp}`;

        try {
            const res = await protocol.parse(startupClient, amcpStr);
            console.log("#" + res.join("\n"));
        } catch (e) {
            console.error("Startup command failed: " + amcpStr);
        }
    }
}
console.log("Initialized startup producers.");

rl.setPrompt("");
rl.prompt();

const server = new AMCPServer(
    5252,
    protocol,
    locks,
    oscSender,
    !config.osc.disableSendToAMCPClients ? config.osc.defaultPort : null
);
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
