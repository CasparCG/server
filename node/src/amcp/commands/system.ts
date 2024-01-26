import type { AMCPCommandEntry } from "../command_repository.js";
import { Native } from "../../native.js";
import { XMLBuilder } from "fast-xml-parser";

export function registerSystemCommands(
    commands: Map<string, AMCPCommandEntry>,
    _channelCommands: Map<string, AMCPCommandEntry>
): void {
    commands.set("VERSION", {
        func: async () => {
            return `201 VERSION OK\r\n${Native.version}\r\n`;
        },
        minNumParams: 0,
    });

    commands.set("DIAG", {
        func: async () => {
            Native.OpenDiag();

            return "202 DIAG OK\r\n";
        },
        minNumParams: 0,
    });

    commands.set("KILL", {
        func: async (context) => {
            setImmediate(() => context.shutdown(false));

            return "202 KILL OK\r\n";
        },
        minNumParams: 0,
    });
    commands.set("RESTART", {
        func: async (context) => {
            setImmediate(() => context.shutdown(true));

            return "202 RESTART OK\r\n";
        },
        minNumParams: 0,
    });

    // repo->register_channel_command(L"Query Commands", L"INFO", info_channel_command, 0);
    // repo->register_command(L"Query Commands", L"INFO", info_command, 0);
    // repo->register_command(L"Query Commands", L"INFO CONFIG", info_config_command, 0);

    commands.set("INFO PATHS", {
        func: async (context) => {
            const xmlBuilder = new XMLBuilder({
                format: true,
            });

            const xmlStr = xmlBuilder.build({
                paths: {
                    "media-path": context.configuration.paths.mediaPath,
                    "log-path": context.configuration.paths.logPath,
                    "data-path": context.configuration.paths.dataPath,
                    "template-path": context.configuration.paths.templatePath,
                    "initial-path": process.cwd(),
                },
            });

            return `201 INFO PATHS OK\r\n${xmlStr}\r\n`;
        },
        minNumParams: 0,
    });

    // repo->register_command(L"Query Commands", L"GL INFO", gl_info_command, 0);
    // repo->register_command(L"Query Commands", L"GL GC", gl_gc_command, 0);

    commands.set("OSC SUBSCRIBE", {
        func: async (context, command) => {
            const port = Number(command.parameters[0]);
            if (isNaN(port)) {
                return "403 OSC SUBSCRIBE BAD PORT\r\n";
            }

            context.osc.addDestinationForOwner(
                command.clientId,
                command.clientAddress,
                port
            );

            return "202 OSC SUBSCRIBE OK\r\n";
        },
        minNumParams: 1,
    });

    commands.set("OSC UNSUBSCRIBE", {
        func: async (context, command) => {
            const port = Number(command.parameters[0]);
            if (isNaN(port)) {
                return "403 OSC UNSUBSCRIBE BAD PORT\r\n";
            }

            context.osc.removeDestinationForOwner(
                command.clientId,
                command.clientAddress,
                port
            );

            return "202 OSC UNSUBSCRIBE OK\r\n";
        },
        minNumParams: 1,
    });
}
