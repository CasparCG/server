import { XMLParser } from "fast-xml-parser";
import { readFile } from "node:fs/promises";
import type {
    CasparCGConfiguration,
    FFmpegConfiguration,
    FlashConfiguration,
    FlashTemplateHost,
    HTMLConfiguration,
    NDIConfiguration,
    PathsConfiguration,
    SystemAudioConfiguration,
} from "./type";

export async function parseXmlConfigFile(
    filePath: string
): Promise<CasparCGConfiguration> {
    const data = await readFile(filePath);

    const parser = new XMLParser({
        preserveOrder: true,
        ignoreAttributes: false,
    });

    const rawObj = parser.parse(data);
    const rawConfig = rawObj?.[1]?.configuration;
    if (!rawConfig) throw new Error("Invalid configuration file");

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

        channels: [],

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

    for (const element of rawConfig) {
        // console.log("parse ", JSON.stringify(element, undefined, 4));
        config.logLevel = extractTextValue(
            element["log-level"],
            "log-level",
            config.logLevel
        );
        config.logAlignColumns = extractBooleanValue(
            element["log-align-columns"],
            "log-align-columns",
            config.logAlignColumns
        );
        if (element["paths"]) {
            parsePathsBlock(config.paths, element["paths"]);
        }
        config.lockClearPhrase = extractTextValue(
            element["lock-clear-phrase"],
            "lock-clear-phrase",
            config.lockClearPhrase
        );

        if (element["template-hosts"]) {
            parseTemplatehosts(config.flash, element["template-hosts"]);
        }
        if (element["flash"]) {
            parseFlashConfiguration(config.flash, element["flash"]);
        }
        if (element["ffmpeg"]) {
            parseFFmpegConfiguration(config.ffmpeg, element["ffmpeg"]);
        }
        if (element["html"]) {
            parseHTMLConfiguration(config.html, element["html"]);
        }
        if (element["system-audio"]) {
            parseSystemAudioConfiguration(
                config.systemAudio,
                element["system-audio"]
            );
        }
        if (element["ndi"]) {
            parseNDIConfiguration(config.ndi, element["ndi"]);
        }

        if (element["video-modes"]) {
            parseVideoModes(config.videoModes, element["video-modes"]);
        }
    }

    return config;
}

function parsePathsBlock(config: PathsConfiguration, elements: any[]) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "paths"');

    for (const element of elements) {
        config.mediaPath = extractTextValue(
            element["media-path"],
            "paths.media-path",
            config.mediaPath
        );
        config.logPath = extractTextValue(
            element["log-path"],
            "paths.log-path",
            config.logPath
        );
        config.dataPath = extractTextValue(
            element["data-path"],
            "paths.data-path",
            config.dataPath
        );
        config.templatePath = extractTextValue(
            element["template-path"],
            "paths.template-path",
            config.templatePath
        );

        if (element["log-path"] && element?.[":@"]?.["@_disabled"] == "true") {
            config.logPath = null;
        }
    }
}

function parseTemplatehosts(config: FlashConfiguration, elements: any[]) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "template-hosts"');

    for (const element of elements) {
        parseTemplatehost(config, element["template-host"]);
    }
}

function parseTemplatehost(config: FlashConfiguration, elements: any[]) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "template-hosts"');

    const templateHost: FlashTemplateHost = {
        videoMode: "",
        filename: "",
        width: 0,
        height: 0,
    };
    config.templateHosts.push(templateHost);

    for (const element of elements) {
        templateHost.videoMode = extractTextValue(
            element["video-mode"],
            "template-hosts.template-host.video-mode",
            templateHost.videoMode
        );
        templateHost.filename = extractTextValue(
            element["filename"],
            "template-hosts.template-host.filename",
            templateHost.filename
        );
        templateHost.width = extractNumberValue(
            element["width"],
            "template-hosts.template-host.width",
            templateHost.width
        );
        templateHost.height = extractNumberValue(
            element["height"],
            "template-hosts.template-host.height",
            templateHost.height
        );
    }
}

function parseFlashConfiguration(config: FlashConfiguration, elements: any[]) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "flash"');

    for (const element of elements) {
        config.enabled = extractBooleanValue(
            element["enabled"],
            "flash.enabled",
            config.enabled
        );

        const bufferDepth = extractTextValue(
            element["buffer-depth"],
            "flash.buffer-depth",
            config.bufferDepth + ""
        );
        config.bufferDepth =
            bufferDepth === "auto" ? "auto" : Number(bufferDepth);

        if (
            typeof config.bufferDepth === "number" &&
            isNaN(config.bufferDepth)
        ) {
            throw new Error(
                `"flash.buffer-depth" value is invalid "${bufferDepth}"`
            );
        }
    }
}

function parseFFmpegConfiguration(
    config: FFmpegConfiguration,
    elements: any[]
) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "ffmpeg"');

    for (const element of elements) {
        if (element["producer"]) {
            parseFFmpegProducerConfiguration(config, element["producer"]);
        }
    }
}

function parseFFmpegProducerConfiguration(
    config: FFmpegConfiguration,
    elements: any[]
) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "ffmpeg.producer"');

    for (const element of elements) {
        config.producer.autoDeinterlace = extractTextValue(
            element["auto-deinterlace"],
            "ffmpeg.producer.auto-deinterlace",
            config.producer.autoDeinterlace
        );

        config.producer.threads = extractNumberValue(
            element["threads"],
            "ffmpeg.producer.threads",
            config.producer.threads
        );
    }
}

function parseHTMLConfiguration(config: HTMLConfiguration, elements: any[]) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "html"');

    for (const element of elements) {
        config.remoteDebuggingPort = extractNumberValue(
            element["remote-debugging-port"],
            "html.remote-debugging-port",
            config.remoteDebuggingPort
        );

        config.enableGpu = extractBooleanValue(
            element["enable-gpu"],
            "html.enable-gpu",
            config.enableGpu
        );

        config.angleBackend = extractTextValue(
            element["angle-backend"],
            "html.angle-backend",
            config.angleBackend
        );
    }
}

function parseSystemAudioConfiguration(
    config: SystemAudioConfiguration,
    elements: any[]
) {
    if (!Array.isArray(elements))
        throw new Error('Invalid configuration structure in "html"');

    for (const element of elements) {
        if (element["producer"]) {
            parseSystemAudioProducerConfiguration(config, element["producer"]);
        }
    }
}

function parseSystemAudioProducerConfiguration(
    config: SystemAudioConfiguration,
    elements: any[]
) {
    if (!Array.isArray(elements))
        throw new Error(
            'Invalid configuration structure in "system-audio.producer"'
        );

    for (const element of elements) {
        config.producer.defaultDeviceName = extractTextValue(
            element["default-device-name"],
            "system-audio.producer.default-device-name",
            config.producer.defaultDeviceName
        );
    }
}

function parseNDIConfiguration(config: NDIConfiguration, elements: any[]) {
    const rootPath = "ndi";
    if (!Array.isArray(elements))
        throw new Error(`Invalid configuration structure in "${rootPath}"`);

    for (const element of elements) {
        config.autoLoad = extractBooleanValue(
            element["auto-load"],
            `${rootPath}.auto-load`,
            config.autoLoad
        );
    }
}

function extractTextValue<T extends string | undefined | null>(
    element: any,
    fullPath: string,
    defaultVal: T
): T {
    if (element && Array.isArray(element)) {
        if (element.length === 0) {
            // TODO - verify this
            return defaultVal;
        }

        if (element.length !== 1)
            throw new Error(
                `Expected a single text value inside of "${fullPath}"`
            );

        const textElm = element.find((elm) => "#text" in elm);
        if (!textElm)
            throw new Error(`Expected a text value inside of "${fullPath}"`);

        return String(textElm["#text"] ?? defaultVal) as T;
    } else {
        return defaultVal;
    }
}

function extractBooleanValue(
    element: any,
    fullPath: string,
    defaultVal: boolean
): boolean {
    if (element && Array.isArray(element)) {
        if (element.length === 0) {
            // TODO - verify this
            return defaultVal;
        }

        if (element.length !== 1)
            throw new Error(
                `Expected a single text value inside of "${fullPath}"`
            );

        const textElm = element.find((elm) => "#text" in elm);
        if (!textElm)
            throw new Error(`Expected a text value inside of "${fullPath}"`);

        const rawVal = textElm["#text"];
        if (rawVal == "") return defaultVal;

        if (typeof textElm["#text"] !== "boolean")
            throw new Error(
                `Cannot interpret "${rawVal}" as boolean in "${fullPath}"`
            );

        return rawVal;
    } else {
        return defaultVal;
    }
}

function extractNumberValue(
    element: any,
    fullPath: string,
    defaultVal: number
): number {
    if (element && Array.isArray(element)) {
        if (element.length === 0) {
            // TODO - verify this
            return defaultVal;
        }

        if (element.length !== 1)
            throw new Error(`Expected a single value inside of "${fullPath}"`);

        const textElm = element.find((elm) => "#text" in elm);
        if (!textElm)
            throw new Error(`Expected a text value inside of "${fullPath}"`);

        const rawVal = textElm["#text"];
        if (rawVal == "") return defaultVal;

        if (typeof textElm["#text"] !== "number")
            throw new Error(
                `Cannot interpret "${rawVal}" as boolean in "${fullPath}"`
            );

        return rawVal;
    } else {
        return defaultVal;
    }
}
