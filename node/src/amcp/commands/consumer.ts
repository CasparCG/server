import type { AMCPCommandEntry } from "../command_repository.js";
import { Native } from "../../native.js";
import { discardError, isChannelIndexValid } from "./util.js";

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
