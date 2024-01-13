import type { AMCPCommandEntry } from "../command_repository.js";
import { Native } from "../../native.js";

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
    // repo->register_command(L"Query Commands", L"INFO PATHS", info_paths_command, 0);
    // repo->register_command(L"Query Commands", L"GL INFO", gl_info_command, 0);
    // repo->register_command(L"Query Commands", L"GL GC", gl_gc_command, 0);

    // repo->register_command(L"Query Commands", L"OSC SUBSCRIBE", osc_subscribe_command, 1);
    // repo->register_command(L"Query Commands", L"OSC UNSUBSCRIBE", osc_unsubscribe_command, 1);
}
