import type {
    AMCPCommandBase,
    AMCPCommandContext,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import {
    discardError,
    isChannelIndexValid,
    isLayerIndexValid,
} from "./util.js";

const CG_DEFAULT_LAYER = 9999;

export function registerProducerCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPCommandEntry>
): void {
    // repo->register_channel_command(L"Template Commands", L"CG ADD", cg_add_command, 3);

    channelCommands.set("CG PLAY", {
        func: async (context, command) => {
            const layerIndex = command.layerIndex ?? CG_DEFAULT_LAYER;
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CG ERROR\r\n";
            }

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod(
                    "play",
                    command.channelIndex + 1,
                    layerIndex,
                    cgLayer
                )
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG STOP", {
        func: async (context, command) => {
            const layerIndex = command.layerIndex ?? CG_DEFAULT_LAYER;
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CG ERROR\r\n";
            }

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod(
                    "stop",
                    command.channelIndex + 1,
                    layerIndex,
                    cgLayer
                )
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG NEXT", {
        func: async (context, command) => {
            const layerIndex = command.layerIndex ?? CG_DEFAULT_LAYER;
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CG ERROR\r\n";
            }

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod(
                    "next",
                    command.channelIndex + 1,
                    layerIndex,
                    cgLayer
                )
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG REMOVE", {
        func: async (context, command) => {
            const layerIndex = command.layerIndex ?? CG_DEFAULT_LAYER;
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CG ERROR\r\n";
            }

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod(
                    "remove",
                    command.channelIndex + 1,
                    layerIndex,
                    cgLayer
                )
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG CLEAR", {
        func: async (context, command) => {
            const layerIndex = command.layerIndex ?? CG_DEFAULT_LAYER;
            if (
                !isChannelIndexValid(context, command.channelIndex) ||
                !isLayerIndexValid(context, command.layerIndex)
            ) {
                return "401 CG ERROR\r\n";
            }

            discardError(
                Native.CallStageMethod(
                    "clear",
                    command.channelIndex + 1,
                    layerIndex
                )
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 0,
    });

    // repo->register_channel_command(L"Template Commands", L"CG UPDATE", cg_update_command, 2);
    // repo->register_channel_command(L"Template Commands", L"CG INVOKE", cg_invoke_command, 2);
}
