import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const lib = require("../../src/build/Release/casparcg.node");

export const Native = lib;

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
