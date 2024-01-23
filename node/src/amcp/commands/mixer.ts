import type {
    AMCPCommandBase,
    AMCPCommandContext,
    AMCPCommandEntry,
    TransformTuple,
} from "../command_repository.js";
import {
    Native,
    NativeImageBlendMode,
    NativeImageChroma,
    NativeImageLevels,
    NativeTransform,
} from "../../native.js";
import type { PartialDeep } from "type-fest";
import { discardError } from "./util.js";

function isChannelIndexValid(
    _context: AMCPCommandContext,
    index: number | null
): index is number {
    // TODO - check if in range
    return index !== null && index >= 0;
}

function isLayerIndexValid(
    _context: AMCPCommandContext,
    index: number | null
): index is number {
    // TODO - check if in range
    return index !== null && index >= 0;
}

export class TransformsApplier {
    readonly #context: AMCPCommandContext;
    readonly #channelIndex: number;
    #transforms: TransformTuple[] = [];

    readonly #defer: boolean;

    constructor(
        context: AMCPCommandContext,
        channelIndex: number,
        params: string[]
    ) {
        this.#context = context;
        this.#channelIndex = channelIndex;

        this.#defer =
            params.length > 0 &&
            params[params.length - 1].toUpperCase() === "DEFER";
        if (this.#defer) {
            params.pop();
        }
    }
    add(
        layerIndex: number,
        fragment: PartialDeep<NativeTransform>,
        duration = 0,
        tween: string = "linear"
    ): void {
        this.#transforms.push({ layerIndex, fragment, duration, tween });
    }

    apply(): void {
        if (this.#transforms.length === 0) return;

        if (this.#defer) {
            const transforms =
                this.#context.deferedTransforms.get(this.#channelIndex) ?? [];
            transforms.push(...this.#transforms);
            this.#context.deferedTransforms.set(this.#channelIndex, transforms);
        } else {
            discardError(
                Native.ApplyTransforms(this.#channelIndex, this.#transforms)
            );
            this.#transforms = [];
        }

        this.#transforms = [];
    }
}

async function runSimpleNonTweenValue(
    context: AMCPCommandContext,
    command: AMCPCommandBase,
    getValue: (transform: NativeTransform) => string,
    setValue: (valueStr: string) => PartialDeep<NativeTransform>
): Promise<string> {
    if (
        !isChannelIndexValid(context, command.channelIndex) ||
        !isLayerIndexValid(context, command.layerIndex)
    ) {
        return `401 ${command.name} ERROR\r\n`;
    }

    if (command.parameters.length === 0) {
        const transform: NativeTransform = await Native.GetLayerMixerProperties(
            command.channelIndex + 1,
            command.layerIndex
        );

        const value = getValue(transform);

        return `201 MIXER OK\r\n${value}\r\n`;
    }

    const transforms = new TransformsApplier(
        context,
        command.channelIndex,
        command.parameters
    );

    if (command.parameters.length === 0)
        throw new Error("Not enough arguments");

    const transform = setValue(command.parameters[0]);

    transforms.add(command.layerIndex, transform);
    transforms.apply();

    return "202 MIXER OK\r\n";
}

async function runSimpleTweenableValue(
    context: AMCPCommandContext,
    command: AMCPCommandBase,
    getValue: (transform: NativeTransform) => string,
    setValue: (valueStr: string) => PartialDeep<NativeTransform>
): Promise<string> {
    const setValue2 = (params: string[]) => setValue(params[0]);

    return runComplexTweenable(context, command, getValue, setValue2, 1);
}

async function runComplexTweenable(
    context: AMCPCommandContext,
    command: AMCPCommandBase,
    getValue: (transform: NativeTransform) => string,
    setValue: (params: string[]) => PartialDeep<NativeTransform>,
    durationIndex: number
): Promise<string> {
    if (
        !isChannelIndexValid(context, command.channelIndex) ||
        !isLayerIndexValid(context, command.layerIndex)
    ) {
        return `401 ${command.name} ERROR\r\n`;
    }

    if (command.parameters.length === 0) {
        const transform: NativeTransform = await Native.GetLayerMixerProperties(
            command.channelIndex + 1,
            command.layerIndex
        );

        const value = getValue(transform);

        return `201 MIXER OK\r\n${value}\r\n`;
    }

    const transforms = new TransformsApplier(
        context,
        command.channelIndex,
        command.parameters
    );

    if (command.parameters.length === 0)
        throw new Error("Not enough arguments");

    const transform = setValue(command.parameters);

    const duration =
        command.parameters.length > durationIndex
            ? Number(command.parameters[durationIndex])
            : 0;
    const tween =
        command.parameters.length > durationIndex + 1
            ? command.parameters[durationIndex + 1]
            : "linear";

    transforms.add(command.layerIndex, transform, duration, tween);
    transforms.apply();

    return "202 MIXER OK\r\n";
}

export function registerMixerCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPCommandEntry>
): void {
    channelCommands.set("MIXER KEYER", {
        func: async (context, command) =>
            runSimpleNonTweenValue(
                context,
                command,
                (transform) => {
                    return transform.image.is_key ? "1" : "0";
                },
                (valueStr) => {
                    const value = Number(valueStr);
                    if (isNaN(value)) throw new Error(`Invalid value`);
                    return {
                        image: {
                            is_key: value >= 1,
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER INVERT", {
        func: async (context, command) =>
            runSimpleNonTweenValue(
                context,
                command,
                (transform) => {
                    return transform.image.invert ? "1" : "0";
                },
                (valueStr) => {
                    const value = Number(valueStr);
                    if (isNaN(value)) throw new Error(`Invalid value`);
                    return {
                        image: {
                            invert: value >= 1,
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER CHROMA", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 MIXER CHROMA ERROR\r\n";
            }

            if (command.parameters.length === 0) {
                const transform: NativeTransform =
                    await Native.GetLayerMixerProperties(
                        command.channelIndex + 1,
                        command.layerIndex
                    );

                const chroma = transform.image.chroma;

                return `201 MIXER OK\r\n${chroma.enable ? 1 : 0} ${
                    chroma.target_hue
                } ${chroma.hue_width} ${chroma.min_saturation} ${
                    chroma.min_brightness
                } ${chroma.softness} ${chroma.spill_suppress} ${
                    chroma.spill_suppress_saturation
                } ${chroma.show_mask ? 1 : 0}\r\n`;
            }

            let duration = 0;
            let tween = "linear";
            let chroma: Partial<NativeImageChroma> = {};

            const legacy_mode = command.parameters[0];
            if (legacy_mode === "none") {
                chroma.enable = false;
            } else if (legacy_mode === "green" || legacy_mode === "blue") {
                chroma.enable = true;
                chroma.hue_width = 0.5 - Number(command.parameters[1]) * 0.5;
                chroma.min_brightness = Number(command.parameters[1]);
                chroma.min_saturation = Number(command.parameters[1]);
                chroma.softness =
                    Number(command.parameters[2]) -
                    Number(command.parameters[1]);
                chroma.spill_suppress =
                    180 - Number(command.parameters[3]) * 180;
                chroma.spill_suppress_saturation = 1;

                chroma.target_hue = legacy_mode === "green" ? 120 : 240;

                if (command.parameters.length > 4) {
                    duration = Number(command.parameters[4]);

                    if (command.parameters.length > 5) {
                        tween = command.parameters[5];
                    }
                }
            } else {
                chroma.enable = command.parameters[0] == "1";

                if (chroma.enable) {
                    chroma.target_hue = Number(command.parameters[1]);
                    chroma.hue_width = Number(command.parameters[2]);
                    chroma.min_saturation = Number(command.parameters[3]);
                    chroma.min_brightness = Number(command.parameters[4]);
                    chroma.softness = Number(command.parameters[5]);
                    chroma.spill_suppress = Number(command.parameters[6]);
                    chroma.spill_suppress_saturation = Number(
                        command.parameters[7]
                    );
                    chroma.show_mask = command.parameters[8] == "1";
                }

                if (command.parameters.length > 9) {
                    duration = Number(command.parameters[9]);

                    if (command.parameters.length > 10) {
                        tween = command.parameters[10];
                    }
                }
            }

            const transforms = new TransformsApplier(
                context,
                command.channelIndex,
                command.parameters
            );
            const value = Number(command.parameters[0]);
            if (isNaN(value)) throw new Error(`Invalid value`);

            transforms.add(
                command.layerIndex,
                {
                    image: {
                        chroma,
                    },
                },
                duration,
                tween
            );
            transforms.apply();

            return "202 MIXER OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("MIXER BLEND", {
        func: async (context, command) =>
            runSimpleNonTweenValue(
                context,
                command,
                (transform) => {
                    return transform.image.blend_mode;
                },
                (valueStr) => {
                    // TODO - validate value
                    return {
                        image: {
                            blend_mode: valueStr as NativeImageBlendMode,
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER OPACITY", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    return transform.image.opacity + "";
                },
                (valueStr) => {
                    return {
                        image: {
                            opacity: Number(valueStr),
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER BRIGHTNESS", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    return transform.image.brightness + "";
                },
                (valueStr) => {
                    return {
                        image: {
                            brightness: Number(valueStr),
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER SATURATION", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    return transform.image.saturation + "";
                },
                (valueStr) => {
                    return {
                        image: {
                            saturation: Number(valueStr),
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER CONTRAST", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    return transform.image.contrast + "";
                },
                (valueStr) => {
                    return {
                        image: {
                            contrast: Number(valueStr),
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER LEVELS", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const levels = transform.image.levels;

                    return `${levels.min_input} ${levels.max_input} ${levels.gamma} ${levels.min_output} ${levels.max_output}`;
                },
                (params) => {
                    const value: NativeImageLevels = {
                        min_input: Number(params[0]),
                        max_input: Number(params[1]),
                        gamma: Number(params[2]),
                        min_output: Number(params[3]),
                        max_output: Number(params[4]),
                    };

                    return {
                        image: {
                            levels: value,
                        },
                    };
                },
                5
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER FILL", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const translation = transform.image.fill_translation;
                    const scale = transform.image.fill_scale;

                    return `${translation[0]} ${translation[1]} ${scale[0]} ${scale[1]}`;
                },
                (params) => {
                    const x = Number(params[0]);
                    const y = Number(params[1]);
                    const x_s = Number(params[2]);
                    const y_s = Number(params[3]);

                    return {
                        image: {
                            fill_translation: [x, y],
                            fill_scale: [x_s, y_s],
                        },
                    };
                },
                4
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER CLIP", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const translation = transform.image.clip_translation;
                    const scale = transform.image.clip_scale;

                    return `${translation[0]} ${translation[1]} ${scale[0]} ${scale[1]}`;
                },
                (params) => {
                    const x = Number(params[0]);
                    const y = Number(params[1]);
                    const x_s = Number(params[2]);
                    const y_s = Number(params[3]);

                    return {
                        image: {
                            clip_translation: [x, y],
                            clip_scale: [x_s, y_s],
                        },
                    };
                },
                4
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER ANCHOR", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const anchor = transform.image.anchor;

                    return `${anchor[0]} ${anchor[1]}`;
                },
                (params) => {
                    const x = Number(params[0]);
                    const y = Number(params[1]);

                    return {
                        image: {
                            anchor: [x, y],
                        },
                    };
                },
                2
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER CROP", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const crop = transform.image.crop;

                    return `${crop.ul[0]} ${crop.ul[1]} ${crop.lr[0]} ${crop.lr[1]}`;
                },
                (params) => {
                    const ul_x = Number(params[0]);
                    const ul_y = Number(params[1]);
                    const lr_x = Number(params[2]);
                    const lr_y = Number(params[3]);

                    return {
                        image: {
                            crop: {
                                ul: [ul_x, ul_y],
                                lr: [lr_x, lr_y],
                            },
                        },
                    };
                },
                2
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER ROTATION", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    const value = (transform.image.angle / Math.PI) * 180;

                    return `${value}`;
                },
                (valueStr) => {
                    const value = (Number(valueStr) * Math.PI) / 180;

                    return {
                        image: {
                            angle: value,
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER PERSPECTIVE", {
        func: async (context, command) =>
            runComplexTweenable(
                context,
                command,
                (transform) => {
                    const perspective = transform.image.perspective;

                    return `${perspective.ul[0]} ${perspective.ul[1]} ${perspective.ur[0]} ${perspective.ur[1]} ${perspective.lr[0]} ${perspective.lr[1]} ${perspective.ll[0]} ${perspective.ll[1]}`;
                },
                (params) => {
                    const ul_x = Number(params[0]);
                    const ul_y = Number(params[1]);
                    const ur_x = Number(params[2]);
                    const ur_y = Number(params[3]);
                    const lr_x = Number(params[4]);
                    const lr_y = Number(params[5]);
                    const ll_x = Number(params[6]);
                    const ll_y = Number(params[7]);

                    return {
                        image: {
                            perspective: {
                                ul: [ul_x, ul_y],
                                ur: [ur_x, ur_y],
                                lr: [lr_x, lr_y],
                                ll: [ll_x, ll_y],
                            },
                        },
                    };
                },
                2
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER VOLUME", {
        func: async (context, command) =>
            runSimpleTweenableValue(
                context,
                command,
                (transform) => {
                    return `${transform.audio.volume}`;
                },
                (valueStr) => {
                    return {
                        audio: {
                            volume: Number(valueStr),
                        },
                    };
                }
            ),
        minNumParams: 0,
    });

    channelCommands.set("MIXER GRID", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return `401 MIXER GRID ERROR\r\n`;
            }

            const transforms = new TransformsApplier(
                context,
                command.channelIndex,
                command.parameters
            );

            const n = Number(command.parameters[0]);
            const delta = 1.0 / n;

            const duration =
                command.parameters.length > 1
                    ? Number(command.parameters[1])
                    : 0;
            const tween =
                command.parameters.length > 2
                    ? command.parameters[2]
                    : "linear";

            for (let x = 0; x < n; ++x) {
                for (let y = 0; y < n; ++y) {
                    const index = x + y * n + 1;
                    transforms.add(
                        index,
                        {
                            image: {
                                fill_translation: [x * delta, y * delta],
                                fill_scale: [delta, delta],
                                clip_translation: [x * delta, y * delta],
                                clip_scale: [delta, delta],
                            },
                        },
                        duration,
                        tween
                    );
                }
            }
            transforms.apply();

            return "202 MIXER OK\r\n";
        },
        minNumParams: 1,
    });

    // channelCommands.set("MIXER MASTERVOLUME", {
    //     func: async (context, command) =>
    //         runSimpleTweenableValue(
    //             context,
    //             command,
    //             (transform) => {
    //                 return `${transform.audio.volume}`;
    //             },
    //             (valueStr) => {
    //                 return {
    //                     audio: {
    //                         volume: Number(valueStr),
    //                     },
    //                 };
    //             }
    //         ),
    //     minNumParams: 0,
    // });

    channelCommands.set("MIXER COMMIT", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return `401 MIXER COMMIT ERROR\r\n`;
            }

            const transforms =
                context.deferedTransforms.get(command.channelIndex) ?? [];
            context.deferedTransforms.delete(command.channelIndex);

            if (transforms.length > 0) {
                // TODO - implement server
                await Native.ApplyTransforms(command.channelIndex, transforms);
            }

            return "202 MIXER OK\r\n";
        },
        minNumParams: 0,
    });

    // channelCommands.set("MIXER CLEAR", {
    //     func: async (context, command) =>
    //         runSimpleTweenableValue(
    //             context,
    //             command,
    //             (transform) => {
    //                 return `${transform.audio.volume}`;
    //             },
    //             (valueStr) => {
    //                 return {
    //                     audio: {
    //                         volume: Number(valueStr),
    //                     },
    //                 };
    //             }
    //         ),
    //     minNumParams: 0,
    // });
}
