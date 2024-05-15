import { AMCPCommandContext } from "../command_repository";

export const DEFAULT_LAYER_INDEX = 0;

export function discardError(val: any) {
    if (
        val instanceof Promise ||
        Object.prototype.hasOwnProperty.call(val, "catch")
    ) {
        val.catch(() => null);
    }
}

export function containsParam(params: string[], check: string): boolean {
    check = check.toUpperCase();

    for (const param of params) {
        if (param.toUpperCase() == check) {
            return true;
        }
    }

    return false;
}

function isArgsToken(message: string): boolean {
    return message.startsWith("(") && message.endsWith(")");
}

export function tokenizeArgs(
    message: string
): Record<string, string | undefined> {
    const tokenMap: Record<string, string | undefined> = {};

    let currentName: string = "";
    let currentValue: string = "";

    let inValue = false;
    let inQuote = false;
    let getSpecialCode = false;

    for (let charIndex = 1; charIndex < message.length; charIndex++) {
        if (!inValue) {
            if (currentName) {
                if (message[charIndex] == " ") {
                    tokenMap[currentName] = "";
                    currentName = "";
                    currentValue = "";
                } else if (message[charIndex] == "=") {
                    currentName = currentName.toUpperCase();
                    inValue = true;
                } else {
                    currentName += message[charIndex];
                }
            }
            continue;
        }
        if (getSpecialCode) {
            // insert code-handling here
            switch (message[charIndex]) {
                case "\\":
                    currentValue += "\\";
                    break;
                case '"':
                    currentValue += '"';
                    break;
                case "n":
                    currentValue += "\n";
                    break;
                default:
                    break;
            }
            getSpecialCode = false;
            continue;
        }
        if (message[charIndex] == "\\") {
            getSpecialCode = true;
            continue;
        }
        if (message[charIndex] == " " && inQuote == false) {
            if (currentValue) {
                tokenMap[currentName] = currentValue;
                currentName = "";
                currentValue = "";
                inValue = false;
            }
            continue;
        }
        if (message[charIndex] == '"') {
            inQuote = !inQuote;
            if (currentValue || !inQuote) {
                tokenMap[currentName] = currentValue;
                currentName = "";
                currentValue = "";
                inValue = false;
            }
            continue;
        }
        currentValue += message[charIndex];
    }

    if (currentValue && currentName) {
        tokenMap[currentName] = currentValue;
    }

    return tokenMap;
}

export interface StingTransitionInfo {
    mask_filename: string;
    overlay_filename: string;
    trigger_point: number;
    audio_fade_start: number;
    audio_fade_duration: number | null;
}

export interface BasicTransitionInfo {
    duration: number;
    direction: "from_left" | "from_right";
    type: "cut" | "mix" | "push" | "slide" | "wipe";
    tweener: string;
}

export function tryMatchStingTransition(
    params: readonly string[]
): StingTransitionInfo | null {
    const result: StingTransitionInfo = {
        mask_filename: "",
        overlay_filename: "",
        trigger_point: 0,
        audio_fade_start: 0,
        audio_fade_duration: null,
    };

    const markerIndex = params.findIndex((p) => p.toUpperCase() === "STING");
    if (markerIndex === -1) {
        return null;
    }

    const stingProps = params.slice(markerIndex + 1);
    if (stingProps.length == 0) {
        // No mask filename
        return null;
    }

    if (isArgsToken(stingProps[0])) {
        const args = tokenizeArgs(stingProps[0]);

        result.mask_filename = args["MASK"] ?? result.mask_filename;
        if (!result.mask_filename) {
            // TODO - throw error?
            // No mask filename
            return null;
        }

        const triggerPoint = Number(args["TRIGGER_POINT"]);
        if (!isNaN(triggerPoint) && triggerPoint > 0) {
            result.trigger_point = triggerPoint;
        }

        result.overlay_filename = args["overlay"] ?? result.overlay_filename;

        const audioFadeStart = Number(args["audio_fade_start"]);
        if (!isNaN(audioFadeStart) && audioFadeStart > 0) {
            result.audio_fade_start = audioFadeStart;
        }

        const audioFadeDuration = Number(args["audio_fade_duration"]);
        if (!isNaN(audioFadeDuration) && audioFadeDuration > 0) {
            result.audio_fade_duration = audioFadeDuration;
        }
        return null;
    } else {
        result.mask_filename = stingProps[0];

        if (stingProps.length >= 2) {
            result.trigger_point = Number(stingProps[1]);
        }

        if (stingProps.length >= 3) {
            result.overlay_filename = stingProps[2];
        }

        return result;
    }
}

export function defaultBasicTransition() {
    const result: BasicTransitionInfo = {
        duration: 0,
        direction: "from_left",
        type: "cut",
        tweener: "linear",
    };
    return result;
}

export function tryMatchBasicTransition(
    params: readonly string[]
): BasicTransitionInfo {
    const result = defaultBasicTransition();

    const message = params.join(" ");

    // TODO: this regex is broken and needs fixing
    const match = message.match(
        /(?<TRANSITION>CUT|PUSH|SLIDE|WIPE|MIX|)\s*(?<DURATION>\d+|)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT|)/i
    );
    // console.log(match);
    if (!match) {
        return result;
    }

    if (match.groups?.["DURATION"]) {
        result.duration = Number(match.groups["DURATION"]);
    }
    if (result.duration <= 0 || isNaN(result.duration)) {
        return result;
    }

    if (match.groups?.["DIRECTION"]) {
        result.direction = match.groups["DIRECTION"] as any;
    }
    if (match.groups?.["TRANSITION"]) {
        result.type = match.groups["TRANSITION"] as any;
    }
    if (match.groups?.["TWEEN"]) {
        result.tweener = match.groups["TWEEN"] as any;
    }

    return result;
}

export function isChannelIndexValid(
    context: AMCPCommandContext,
    index: number | null
): index is number {
    return (
        typeof index === "number" && index >= 0 && index < context.channelCount
    );
}

export function isLayerIndexValid(
    _context: AMCPCommandContext,
    index: number | null
): index is number {
    // TODO - check if in range
    return index !== null && index >= 0;
}
