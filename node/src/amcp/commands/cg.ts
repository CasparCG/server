import type {
    AMCPChannelCommandEntry,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import { discardError } from "./util.js";

const CG_DEFAULT_LAYER = 9999;

export function registerProducerCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPChannelCommandEntry>
): void {
    // repo->register_channel_command(L"Template Commands", L"CG ADD", cg_add_command, 3);

    channelCommands.set("CG PLAY", {
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod("play", channelIndex, layerIndex, cgLayer)
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG STOP", {
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod("stop", channelIndex, layerIndex, cgLayer)
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG NEXT", {
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod("next", channelIndex, layerIndex, cgLayer)
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG REMOVE", {
        func: async (_context, command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

            const cgLayer = Number(command.parameters[0]);

            discardError(
                Native.CallCgMethod("remove", channelIndex, layerIndex, cgLayer)
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("CG CLEAR", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

            discardError(
                Native.CallStageMethod("clear", channelIndex, layerIndex)
            );

            return "202 CG OK\r\n";
        },
        minNumParams: 0,
    });

    // repo->register_channel_command(L"Template Commands", L"CG UPDATE", cg_update_command, 2);
    // repo->register_channel_command(L"Template Commands", L"CG INVOKE", cg_invoke_command, 2);
}
