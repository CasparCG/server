export enum VideoMode {
    PAL = "PAL",
    NTSC = "NTSC",
    _576p2500 = "576p2500",
    _720p2398 = "720p2398",
    _720p2400 = "720p2400",
    _720p2500 = "720p2500",
    _720p5000 = "720p5000",
    _720p2997 = "720p2997",
    _720p5994 = "720p5994",
    _720p3000 = "720p3000",
    _720p6000 = "720p6000",
    _1080p2398 = "1080p2398",
    _1080p2400 = "1080p2400",
    _1080i5000 = "1080i5000",
    _1080i5994 = "1080i5994",
    _1080i6000 = "1080i6000",
    _1080p2500 = "1080p2500",
    _1080p2997 = "1080p2997",
    _1080p3000 = "1080p3000",
    _1080p5000 = "1080p5000",
    _1080p5994 = "1080p5994",
    _1080p6000 = "1080p6000",
    _1556p2398 = "1556p2398",
    _1556p2400 = "1556p2400",
    _1556p2500 = "1556p2500",
    dci1080p2398 = "dci1080p2398",
    dci1080p2400 = "dci1080p2400",
    dci1080p2500 = "dci1080p2500",
    _2160p2398 = "2160p2398",
    _2160p2400 = "2160p2400",
    _2160p2500 = "2160p2500",
    _2160p2997 = "2160p2997",
    _2160p3000 = "2160p3000",
    _2160p5000 = "2160p5000",
    _2160p5994 = "2160p5994",
    _2160p6000 = "2160p6000",
    dci2160p2398 = "dci2160p2398",
    dci2160p2400 = "dci2160p2400",
    dci2160p2500 = "dci2160p2500",
}

export interface CasparCGConfiguration {
    logLevel: "trace" | "debug" | "info" | "warning" | "error" | "fatal";
    logAlignColumns: boolean;

    paths: PathsConfiguration;
    lockClearPhrase: string;

    flash: FlashConfiguration;
    ffmpeg: FFmpegConfiguration;
    html: HTMLConfiguration;
    systemAudio: SystemAudioConfiguration;
    ndi: NDIConfiguration;

    videoModes: CustomVideoMode[];

    channels: ChannelConfiguration[];

    controllers: ControllerConfiguration[];
    osc: OSCConfiguration;
    mediaScanner: MediaScannerConfiguration;
}

export interface PathsConfiguration {
    mediaPath: string;
    logPath: string | null;
    dataPath: string;
    templatePath: string;
}

export interface FlashConfiguration {
    enabled: boolean;
    bufferDepth: "auto" | number;
    templateHosts: FlashTemplateHost[];
}

export interface FlashTemplateHost {
    videoMode: string;
    filename: string;
    width: number;
    height: number;
}

export interface FFmpegConfiguration {
    producer: {
        autoDeinterlace: "none" | "interlaced" | "all";
        threads: number;
    };
}

export interface HTMLConfiguration {
    remoteDebuggingPort: number;
    enableGpu: boolean;
    angleBackend: "gl" | "d3d11" | "d3d9";
}

export interface SystemAudioConfiguration {
    producer: {
        defaultDeviceName: string | null;
    };
}

export interface NDIConfiguration {
    autoLoad: boolean;
}

export interface CustomVideoMode {
    id: string;
    width: number;
    height: number;
    fieldCount: number;
    timeScale: number;
    duration: number;
    cadence: string;
}

export interface ChannelConfiguration {
    videoMode: VideoMode | string;
    consumers: ChannelConsumerConfiguration[];
    producers: Record<number, string>; // AMCP command string
}

// Danger: these get passed directly into the consumers, so casing needs to match what they expect
export type ChannelConsumerConfiguration =
    | DecklinkConsumerConfiguration
    | BluefishConsumerConfiguration
    | SystemAudioConsumerConfiguration
    | ScreenConsumerConfiguration
    | NDIConsumerConfiguration
    | FFmpegConsumerConfiguration
    | ArtnetConsumerConfiguration;

export interface DecklinkConsumerConfiguration {
    type: "decklink";
    device: number;
    "key-device": number;
    "embedded-audio": boolean;
    latency: "normal" | "low" | "default";
    keyer: "external" | "external_separate_device" | "internal" | "default";
    "key-only": boolean;
    "buffer-depth": number;
    "video-mode": VideoMode;
    subregion: DecklinkConsumerSubregion;

    "wait-for-reference": "auto" | "enable" | "disable";
    "wait-for-reference-duration": number; // Seconds

    ports: DecklinkConsumerPort[];
}

export interface DecklinkConsumerPort {
    device: number;
    "key-only": boolean;
    "video-mode": VideoMode;
    subregion: DecklinkConsumerSubregion;
}

export interface DecklinkConsumerSubregion {
    "src-x": number;
    "src-y": number;
    "dest-x": number;
    "dest-y": number;
    width: number;
    height: number;
}

export interface BluefishConsumerConfiguration {
    type: "bluefish";
    device: number;
    "sdi-stream": number;
    "embedded-audio": boolean;
    keyer: "external" | "internal" | "disabled";
    "internal-keyer-audio-source": "videooutputchannel" | "sdivideoinput";
    watchdog: number;
    "uhd-mode": 0 | 1 | 2 | 3;
}

export interface SystemAudioConsumerConfiguration {
    type: "system-audio";
    "channel-layout": "stereo" | "mono" | "matrix";
    latency: number;
}

export interface ScreenConsumerConfiguration {
    type: "screen";
    device: number;
    "aspect-ratio": "default" | "4:3" | "16:9";
    stretch: "none" | "fill" | "uniform" | "uniform_to_fill";
    windowed: boolean;
    "key-only": boolean;
    vsync: boolean;
    borderless: boolean;
    "always-on-top": boolean;

    x: number;
    y: number;
    width: number;
    height: number;

    "sbs-key": boolean;
    "colour-space": "RGB" | "datavideo-full" | "datavideo-limited";
}

export interface NDIConsumerConfiguration {
    type: "ndi";
    name: string;
    "allow-fields": boolean;
}

export interface FFmpegConsumerConfiguration {
    type: "ffmpeg";
    path: string;
    args: string;
}

export interface ArtnetConsumerConfiguration {
    type: "artnet";
    universe: number;
    host: string;
    port: number;

    "refresh-rate": number;

    fixtures: ArtnetConsumerFixture[];
}

export interface ArtnetConsumerFixture {
    type: "RGBW";
    "start-address": number;
    "fixture-count": number;
    "fixture-channels": number;

    x: number;
    y: number;
    width: number;
    height: number;

    rotation: number;
}

export interface ControllerConfiguration {
    type: "tcp";
    port: number;
    protocol: "AMCP";
}

export interface OSCConfiguration {
    defaultPort: number;
    disableSendToAMCPClients: boolean;
    predefinedClients: PredefinedOSCClient[];
}

export interface PredefinedOSCClient {
    address: string;
    port: number;
}

export interface MediaScannerConfiguration {
    host: string;
    port: number;
}
