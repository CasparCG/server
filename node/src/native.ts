import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const lib = require("../../src/build/Release/casparcg.node");

export const Native: Readonly<NativeApi> = lib;

export interface NativeApi {
    readonly version: string;

    init(
        config: ServerInitConfig,
        updatedState?: (
            channelId: number,
            newState: Record<string, any[]>
        ) => void
    ): void;
    stop(): void;

    ConfigAddCustomVideoFormat(format: NativeCustomVideoFormat): void;

    CreateProducer(
        channelIndex: number,
        layerIndex: number,
        parameters: string[]
    ): NativeUnusedProducer | null;
    CreateStingTransition(
        channelIndex: number,
        layerIndex: number,
        producer: NativeUnusedProducer,
        props: NativeStingProps
    ): NativeUnusedProducer | null;
    CreateBasicTransition(
        channelIndex: number,
        layerIndex: number,
        producer: NativeUnusedProducer,
        props: NativeTransitionProps
    ): NativeUnusedProducer | null;

    AddConsumer(
        channelIndex: number,
        layerIndex: number | null,
        parameters: string[]
    ): Promise<number>;
    RemoveConsumerByPort(channelIndex: number, layerIndex: number): boolean;
    RemoveConsumerByParams(channelIndex: number, parameters: string[]): boolean;

    AddChannelConsumer(
        channelIndex: number,
        config: Record<string, any>
    ): Promise<number>;
    AddChannel(video_mode: string): number;

    CallStageMethod(
        operation: "play",
        channelIndex: number,
        layerIndex: number
    ): Promise<void>;
    CallStageMethod(
        operation: "preview",
        channelIndex: number,
        layerIndex: number
    ): Promise<void>;
    CallStageMethod(
        operation: "load",
        channelIndex: number,
        layerIndex: number,
        producer: NativeUnusedProducer | null,
        preview: boolean,
        autoPlay: boolean
    ): Promise<void>;
    CallStageMethod(
        operation: "pause",
        channelIndex: number,
        layerIndex: number
    ): Promise<void>;
    CallStageMethod(
        operation: "resume",
        channelIndex: number,
        layerIndex: number
    ): Promise<void>;
    CallStageMethod(
        operation: "stop",
        channelIndex: number,
        layerIndex: number
    ): Promise<void>;
    CallStageMethod(
        operation: "clear",
        channelIndex: number,
        layerIndex: number | null
    ): Promise<void>;
    CallStageMethod(
        operation: "call",
        channelIndex: number,
        layerIndex: number,
        parameters: string[]
    ): Promise<string>;
    CallStageMethod(
        operation: "swapChannel",
        channelIndex1: number,
        channelIndex2: number,
        swapTransforms?: boolean
    ): Promise<void>;
    CallStageMethod(
        operation: "swapLayer",
        channelIndex1: number,
        layerIndex1: number,
        channelIndex2: number,
        layerIndex2: number,
        swapTransforms?: boolean
    ): Promise<void>;

    CallCgMethod(
        operation: "add",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number,
        filename: string,
        doStart: boolean,
        label: string,
        payload: string
    ): Promise<void>;
    CallCgMethod(
        operation: "play",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number
    ): Promise<void>;
    CallCgMethod(
        operation: "stop",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number
    ): Promise<void>;
    CallCgMethod(
        operation: "next",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number
    ): Promise<void>;
    CallCgMethod(
        operation: "remove",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number
    ): Promise<void>;
    CallCgMethod(
        operation: "invoke",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number,
        invoke: string
    ): Promise<string>;
    CallCgMethod(
        operation: "update",
        channelIndex: number,
        layerIndex: number,
        cgLayer: number,
        payload: string
    ): Promise<void>;

    SetChannelFormat(channelIndex: number, videoMode: string): Promise<void>;
    // exports.Set(Napi::String::New(env, "GetLayerMixerProperties"), Napi::Function::New(env, GetLayerMixerProperties));
    // exports.Set(Napi::String::New(env, "ApplyTransforms"), Napi::Function::New(env, ApplyTransforms));

    OpenDiag(): void;
    FindCaseInsensitive(filepath: string): Promise<string | null>;
}

export interface ServerInitConfig {
    paths: null;
    moduleConfig?: {
        ffmpeg?: {
            producer?: {
                threads?: number;
                "auto-deinterlace"?: "interlaced" | "none" | "all";
            };
        };
        flash?: {
            enabled?: boolean;
            "buffer-depth"?: number;
            templateHosts?: {
                "video-mode": string;
                filename: string;
                width: number;
                height: number;
            }[];
        };
        html?: {
            "enable-gpu"?: boolean;
            "angle-backend"?: "gl" | "d3d11";
            "remote-debugging-port"?: number;
        };
        ndi?: {
            "auto-load"?: boolean;
        };
        "system-audio"?: {
            producer?: {
                "default-device-name"?: string;
            };
        };
    };
}

export interface NativeUnusedProducer {
    IsValid(): boolean;
}

export interface NativeStingProps {
    mask_filename: string;
    overlay_filename: string;
    trigger_point: number;
    audio_fade_start: number;
    audio_fade_duration: number | null;
}

export interface NativeTransitionProps {
    duration: number;
    direction: string;
    type: string;
    tweener: string;
}

export interface NativeCustomVideoFormat {
    id: string;
    width: number;
    height: number;
    fieldCount: number;
    timeScale: number;
    duration: number;
    cadence: string;
}

export interface NativeTransform {
    audio: NativeAudioTransform;
    image: NativeImageTransform;
}
export interface NativeAudioTransform {
    volume: number;
}
export interface NativeImageTransform {
    opacity: number;
    contrast: number;
    brightness: number;
    saturation: number;
    anchor: [number, number];
    fill_translation: [number, number];
    fill_scale: [number, number];
    clip_translation: [number, number];
    clip_scale: [number, number];
    angle: number;
    crop: NativeImageRectangle;
    perspective: NativeImageCorners;
    levels: NativeImageLevels;
    chroma: NativeImageChroma;
    is_key: boolean;
    invert: boolean;
    is_mix: boolean;
    blend_mode: NativeImageBlendMode;
    layer_depth: number;
}

export interface NativeImageRectangle {
    ul: [number, number];
    lr: [number, number];
}
export interface NativeImageCorners {
    ul: [number, number];
    ur: [number, number];
    lr: [number, number];
    ll: [number, number];
}
export interface NativeImageLevels {
    min_input: number;
    max_input: number;
    gamma: number;
    min_output: number;
    max_output: number;
}
export interface NativeImageChroma {
    enable: boolean;
    show_mask: boolean;
    target_hue: number;
    hue_width: number;
    min_saturation: number;
    min_brightness: number;
    softness: number;
    spill_suppress: number;
    spill_suppress_saturation: number;
}

export enum NativeImageBlendMode {
    normal = "normal",
    lighten = "lighten",
    darken = "darken",
    multiply = "multiply",
    average = "average",
    add = "add",
    subtract = "subtract",
    difference = "difference",
    negation = "negation",
    exclusion = "exclusion",
    screen = "screen",
    overlay = "overlay",
    soft_light = "soft_light",
    hard_light = "hard_light",
    color_dodge = "color_dodge",
    color_burn = "color_burn",
    linear_dodge = "linear_dodge",
    linear_burn = "linear_burn",
    linear_light = "linear_light",
    vivid_light = "vivid_light",
    pin_light = "pin_light",
    hard_mix = "hard_mix",
    reflect = "reflect",
    glow = "glow",
    phoenix = "phoenix",
    contrast = "contrast",
    saturation = "saturation",
    color = "color",
    luminosity = "luminosity",
    mix = "mix",
    // blend_mode_count,
}
