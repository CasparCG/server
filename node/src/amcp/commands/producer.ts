import type {
    AMCPChannelCommandEntry,
    AMCPCommandBase,
    AMCPCommandContext,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import {
    DEFAULT_LAYER_INDEX,
    containsParam,
    defaultBasicTransition,
    discardError,
    tryMatchBasicTransition,
    tryMatchStingTransition,
} from "./util.js";

export function registerProducerCommands(
    commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPChannelCommandEntry>
): void {
    async function loadBgCommand(
        _context: AMCPCommandContext,
        command: AMCPCommandBase,
        channelIndex: number,
        layerIndex: number | null
    ): Promise<string> {
        layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

        const autoPlay = containsParam(command.parameters, "AUTO");

        const producer = await Native.CreateProducer(
            channelIndex,
            layerIndex,
            command.parameters
        );
        if (!producer || !producer.IsValid()) {
            if (containsParam(command.parameters, "CLEAR_ON_404")) {
                discardError(
                    Native.CallStageMethod(
                        "load",
                        channelIndex,
                        layerIndex,
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
                channelIndex,
                layerIndex,
                producer,
                stingProps
            );
        }

        if (!transitionProducer) {
            const transitionProps = tryMatchBasicTransition(command.parameters);
            transitionProducer = await Native.CreateBasicTransition(
                channelIndex,
                layerIndex,
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
                channelIndex,
                layerIndex,
                transitionProducer,
                false,
                autoPlay
            ) // TODO: LOOP
        );
        return "202 LOADBG OK\r\n";
    }

    channelCommands.set("LOADBG", {
        func: async (context, command, channelIndex, layerIndex) => {
            return loadBgCommand(context, command, channelIndex, layerIndex);
        },
        minNumParams: 1,
    });

    channelCommands.set("LOAD", {
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            if (command.parameters.length === 0) {
                // TODO
                discardError(
                    Native.CallStageMethod("preview", channelIndex, layerIndex)
                );

                return "202 LOAD OK\r\n";
            } else {
                // TODO
                const producer = await Native.CreateProducer(
                    channelIndex,
                    layerIndex,
                    command.parameters
                );
                if (!producer || !producer.IsValid()) {
                    if (containsParam(command.parameters, "CLEAR_ON_404")) {
                        discardError(
                            Native.CallStageMethod(
                                "load",
                                channelIndex,
                                layerIndex,
                                null, // aka 'EMPTY'
                                true,
                                false
                            )
                        );
                    }

                    const filename = command.parameters[0] ?? "";
                    throw new Error(`${filename} not found`); // TODO file_not_found
                }

                const transitionProps = defaultBasicTransition();
                const transitionProducer = await Native.CreateBasicTransition(
                    channelIndex,
                    layerIndex,
                    producer,
                    transitionProps
                );

                discardError(
                    Native.CallStageMethod(
                        "load",
                        channelIndex,
                        layerIndex,
                        transitionProducer,
                        true,
                        false
                    )
                );

                return "202 LOAD OK\r\n";
            }
        },
        minNumParams: 0,
    });

    channelCommands.set("PLAY", {
        func: async (context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            if (command.parameters.length > 0) {
                await loadBgCommand(context, command, channelIndex, layerIndex);
            }

            discardError(
                Native.CallStageMethod("play", channelIndex, layerIndex)
            );

            return "202 PLAY OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("PAUSE", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            discardError(
                Native.CallStageMethod("pause", channelIndex, layerIndex)
            );
            return "202 PAUSE OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("RESUME", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            discardError(
                Native.CallStageMethod("resume", channelIndex, layerIndex)
            );
            return "202 RESUME OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("STOP", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            discardError(
                Native.CallStageMethod("stop", channelIndex, layerIndex)
            );
            return "202 STOP OK\r\n";
        },
        minNumParams: 0,
    });

    channelCommands.set("CLEAR", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            discardError(
                Native.CallStageMethod("clear", channelIndex, layerIndex)
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
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? DEFAULT_LAYER_INDEX;

            try {
                const result = await Native.CallStageMethod(
                    "call",
                    channelIndex,
                    layerIndex,
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
        func: async (_context, command, channelIndex, layerIndex) => {
            const swapTransforms =
                command.parameters[1]?.toUpperCase() === "TRANSFORMS";

            if (layerIndex !== null) {
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
                        channelIndex,
                        layerIndex,
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
                        channelIndex,
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
