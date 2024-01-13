import { createInterface } from "node:readline";
import { Native } from "./native.js";
import { AMCPClientBatchInfo, AMCPProtocolStrategy } from "./amcp/protocol.js";
import { ChannelLocks } from "./amcp/channel_locks.js";
import { Client } from "./amcp/client.js";
import { AMCPServer } from "./amcp/server.js";
import { CasparCGConfiguration, VideoMode } from "./config/type.js";
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
        mediaPath: "media/",
        logPath: "log/",
        dataPath: "data/",
        templatePath: "template/",
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
                    aspectRatio: "default",
                    stretch: "none",
                    windowed: true,
                    keyOnly: false,
                    vsync: false,
                    borderless: false,
                    alwaysOnTop: false,

                    x: 0,
                    y: 0,
                    width: 0,
                    height: 0,

                    sbsKey: false,
                    colourSpace: "RGB",
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

console.log(Native);
Native.init(config);

for (const videoMode of config.videoModes) {
    Native.ConfigAddCustomVideoFormat(videoMode);
}
console.log("Initialized video modes.");

if (!Native.ConfigAddOscPredefinedClient("127.0.0.1", 6250)) {
    console.log("failed to add osc client");
}

const locks = new ChannelLocks("");
const protocol = new AMCPProtocolStrategy(locks);

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

// setInterval(() => {
//     // Keep the app alive
// }, 10000);

rl.setPrompt("");
rl.prompt();

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
