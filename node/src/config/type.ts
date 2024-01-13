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
    timeScale: number;
    duration: number;
    cadence: number;
}

export interface ChannelConfiguration {
    videoMode: VideoMode | string;
    consumers: ChannelConsumerConfiguration[];
    producers: Record<number, string>; // AMCP command string
}

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
    keyDevice: number;
    embeddedAudio: boolean;
    latency: "normal" | "low" | "default";
    keyer: "external" | "external_separate_device" | "internal" | "default";
    keyOnly: boolean;
    bufferDepth: number;
    videoMode: VideoMode;
    subRegion: DecklinkConsumerSubregion;

    waitForReference: "auto" | "enable" | "disable";
    waitForReferenceDuration: number; // Seconds

    ports: DecklinkConsumerPort[];
}

export interface DecklinkConsumerPort {
    device: number;
    keyOnly: boolean;
    videoMode: VideoMode;
    subRegion: DecklinkConsumerSubregion;
}

export interface DecklinkConsumerSubregion {
    srcX: number;
    srcY: number;
    destX: number;
    destY: number;
    width: number;
    height: number;
}

export interface BluefishConsumerConfiguration {
    type: "bluefish";
    device: number;
    sdiStream: number;
    embeddedAudio: boolean;
    keyer: "external" | "internal" | "disabled";
    internalKeyerAudioSource: "videooutputchannel" | "sdivideoinput";
    watchdog: number;
    uhdMode: 0 | 1 | 2 | 3;
}

export interface SystemAudioConsumerConfiguration {
    type: "system-audio";
    channelLayout: "stereo" | "mono" | "matrix";
    latency: number;
}

export interface ScreenConsumerConfiguration {
    type: "screen";
    device: number;
    aspectRatio: "default" | "4:3" | "16:9";
    stretch: "none" | "fill" | "uniform" | "uniform_to_fill";
    windowed: boolean;
    keyOnly: boolean;
    vsync: boolean;
    borderless: boolean;
    alwaysOnTop: boolean;

    x: number;
    y: number;
    width: number;
    height: number;

    sbsKey: boolean;
    colourSpace: "RGB" | "datavideo-full" | "datavideo-limited";
}

export interface NDIConsumerConfiguration {
    type: "ndi";
    name: string;
    allowFields: boolean;
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

    refreshRate: number;

    fixtures: ArtnetConsumerFixture[];
}

export interface ArtnetConsumerFixture {
    type: "RGBW";
    startAddress: number;
    fixtureCount: number;
    fixtureChannels: number;

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
