import type {
    AMCPCommandContext,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import {
    containsParam,
    tryMatchBasicTransition,
    tryMatchStingTransition,
} from "./util.js";

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

function discardError(val: any) {
    if (
        val instanceof Promise ||
        Object.prototype.hasOwnProperty.call(val, "catch")
    ) {
        val.catch(() => null);
    }
}
export function registerProducerCommands(
    commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPCommandEntry>
): void {
    channelCommands.set("LOADBG", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 PAUSE ERROR\r\n";
            }

            const autoPlay = containsParam(command.parameters, "AUTO");

            console.log("parame", command.parameters);

            const producer = await Native.CreateProducer(
                command.channelIndex + 1,
                command.layerIndex,
                command.parameters
            );
            if (!producer || !producer.IsValid()) {
                if (containsParam(command.parameters, "CLEAR_ON_404")) {
                    discardError(
                        Native.CallStageMethod(
                            "load",
                            command.channelIndex + 1,
                            command.layerIndex,
                            null, // aka 'EMPTY'
                            false,
                            autoPlay
                        )
                    );
                }

                const filename = command.parameters[0] ?? "";
                throw new Error(`${filename} not found`); // TODO file_not_found
            }

            let transitionProducer: any = null;

            const stingProps = tryMatchStingTransition(command.parameters);
            if (stingProps && !transitionProducer) {
                transitionProducer = await Native.CreateStingTransition(
                    command.channelIndex + 1,
                    command.layerIndex,
                    producer,
                    stingProps
                );
            }

            if (!transitionProducer) {
                const transitionProps = tryMatchBasicTransition(
                    command.parameters
                );
                console.log("props", transitionProps);
                transitionProducer = await Native.CreateBasicTransition(
                    command.channelIndex + 1,
                    command.layerIndex,
                    producer,
                    transitionProps
                );
            }

            if (!transitionProducer) {
                const filename = command.parameters[0] ?? "";
                throw new Error(`${filename} failed`); // TODO file_not_found
            }

            // TODO - we should pass the format into load(), so that we can catch it having changed since the producer was
            // initialised
            discardError(
                Native.CallStageMethod(
                    "load",
                    command.channelIndex + 1,
                    command.layerIndex,
                    transitionProducer,
                    false,
                    autoPlay
                ) // TODO: LOOP
            );
            return "202 LOADBG OK\r\n";
        },
        minNumParams: 1,
    });
    // repo->register_channel_command(L"Basic Commands", L"LOADBG", loadbg_command, 1);
    // repo->register_channel_command(L"Basic Commands", L"LOAD", load_command, 0);
    // repo->register_channel_command(L"Basic Commands", L"PLAY", play_command, 0);

    channelCommands.set("PAUSE", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 PAUSE ERROR\r\n";
            }

            discardError(
                Native.CallStageMethod(
                    "pause",
                    command.channelIndex + 1,
                    command.layerIndex
                )
            );
            return "202 PAUSE OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("RESUME", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 RESUME ERROR\r\n";
            }

            discardError(
                Native.CallStageMethod(
                    "resume",
                    command.channelIndex + 1,
                    command.layerIndex
                )
            );
            return "202 RESUME OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("STOP", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 STOP ERROR\r\n";
            }

            discardError(
                Native.CallStageMethod(
                    "stop",
                    command.channelIndex + 1,
                    command.layerIndex
                )
            );
            return "202 STOP OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("CLEAR", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return "401 CLEAR ERROR\r\n";
            }

            discardError(
                Native.CallStageMethod(
                    "clear",
                    command.channelIndex + 1,
                    command.layerIndex
                )
            );
            return "202 CLEAR OK\r\n";
        },
        minNumParams: 0,
    });

    commands.set("CLEAR ALL", {
        func: async (context) => {
            for (let i = 0; i < context.channelCount; i++) {
                discardError(Native.CallStageMethod("clear", i + 1));
            }

            return "202 CLEAR ALL OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("CALL", {
        func: async (context, command) => {
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CALL ERROR\r\n";
            }

            try {
                const result = await Native.CallStageMethod(
                    "call",
                    command.channelIndex + 1,
                    command.layerIndex,
                    command.parameters
                );
                if (result) {
                    return "202 CALL OK\r\n" + result + "\r\n";
                } else {
                    return "202 CALL OK\r\n";
                }
            } catch (e) {
                return "501 CALL FAILED\r\n";
            }
        },
        minNumParams: 1,
    });

    channelCommands.set("SWAP", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return "401 SWAP ERROR\r\n";
            }

            const swapTransforms =
                command.parameters[1]?.toUpperCase() === "TRANSFORMS";

            if (command.layerIndex !== null) {
                const strs = command.parameters[0]
                    .split("-")
                    .map((v) => Number(v));
                if (strs.length != 2) {
                    console.error(
                        `SWAP: Invalid channel-layer pair ${command.parameters[0]}`
                    );
                    return "402 SWAP ERROR\r\n";
                }

                discardError(
                    Native.CallStageMethod(
                        "swapLayer",
                        command.channelIndex + 1,
                        command.layerIndex,
                        strs[0],
                        strs[1],
                        swapTransforms
                    )
                );
                return "202 SWAP OK\r\n";
            } else {
                const otherChannelIndex = Number(command.parameters[0]);
                if (isNaN(otherChannelIndex)) {
                    console.error(
                        `SWAP: Bad channel id ${command.parameters[0]}`
                    );
                    return "401 SWAP ERROR\r\n";
                }

                discardError(
                    Native.CallStageMethod(
                        "swapChannel",
                        command.channelIndex + 1,
                        otherChannelIndex,
                        swapTransforms
                    )
                );
                return "202 SWAP OK\r\n";
            }
        },
        minNumParams: 1,
    });
}
