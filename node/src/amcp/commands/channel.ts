import type { AMCPCommandEntry } from "../command_repository.js";
import { Native } from "../../native.js";

export function registerChannelCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPCommandEntry>
): void {
    channelCommands.set("SET MODE", {
        func: async (_context, command) => {
            const formatName = command.parameters[0];
            try {
                Native.SetChannelFormat(command.channelIndex, formatName);
            } catch (e) {
                throw new Error(`Invalid video mode`); // TODO user_error
            }
            return "202 SET MODE OK\r\n";
        },
        minNumParams: 1,
    });
}
