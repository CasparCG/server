import type {
    AMCPCommandContext,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import { discardError } from "./util.js";

function isChannelIndexValid(
    _context: AMCPCommandContext,
    index: number | null
): index is number {
    // TODO - check if in range
    return index !== null && index >= 0;
}

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
    channelCommands: Map<string, AMCPCommandEntry>
): void {
    // repo->register_channel_command(L"Basic Commands", L"PRINT", print_command, 0);

    channelCommands.set("ADD", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return "401 ADD ERROR\r\n";
            }

            const params = replacePlaceholders(
                command.parameters,
                "<CLIENT_IP_ADDRESS>",
                command.clientAddress
            );

            discardError(
                Native.AddConsumer(
                    command.channelIndex + 1,
                    command.layerIndex,
                    params
                )
            );

            return "202 ADD OK\r\n";
        },
        minNumParams: 1,
    });

    channelCommands.set("REMOVE", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return "401 ADD ERROR\r\n";
            }

            if (command.layerIndex !== null) {
                discardError(
                    Native.RemoveConsumerByPort(
                        command.channelIndex + 1,
                        command.layerIndex
                    )
                );

                return "202 REMOVE OK\r\n";
            } else {
                const params = replacePlaceholders(
                    command.parameters,
                    "<CLIENT_IP_ADDRESS>",
                    command.clientAddress
                );

                discardError(
                    Native.RemoveConsumerByParams(
                        command.channelIndex + 1,
                        params
                    )
                );

                return "202 REMOVE OK\r\n";
            }
        },
        minNumParams: 1,
    });

    channelCommands.set("PRINT", {
        func: async (context, command) => {
            if (!isChannelIndexValid(context, command.channelIndex)) {
                return "401 ADD ERROR\r\n";
            }

            discardError(
                Native.AddConsumer(
                    command.channelIndex + 1,
                    command.layerIndex,
                    ["IMAGE"]
                )
            );

            return "202 PRINT OK\r\n";
        },
        minNumParams: 0,
    });
}
