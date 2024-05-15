import type {
    AMCPChannelCommandEntry,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import { discardError } from "./util.js";

function replacePlaceholders(
    params: string[],
    placeholder: string,
    replacement: string
): string[] {
    if (!placeholder) return params;

    return params.map((param) => param.replace(placeholder, replacement));
}

export function registerConsumerCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPChannelCommandEntry>
): void {
    channelCommands.set("ADD", {
        func: async (_context, command, channelIndex, layerIndex) => {
            const params = replacePlaceholders(
                command.parameters,
                "<CLIENT_IP_ADDRESS>",
                command.clientAddress
            );

            discardError(Native.AddConsumer(channelIndex, layerIndex, params));

            return "202 ADD OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("REMOVE", {
        func: async (_context, command, channelIndex, layerIndex) => {
            if (layerIndex !== null) {
                discardError(
                    Native.RemoveConsumerByPort(channelIndex, layerIndex)
                );

                return "202 REMOVE OK\r\n";
            } else {
                const params = replacePlaceholders(
                    command.parameters,
                    "<CLIENT_IP_ADDRESS>",
                    command.clientAddress
                );

                discardError(
                    Native.RemoveConsumerByParams(channelIndex, params)
                );

                return "202 REMOVE OK\r\n";
            }
        },
        minNumParams: 1,
    });

    channelCommands.set("PRINT", {
        func: async (_context, _command, channelIndex, layerIndex) => {
            discardError(
                Native.AddConsumer(channelIndex, layerIndex, ["IMAGE"])
            );

            return "202 PRINT OK\r\n";
        },
        minNumParams: 0,
    });
}
