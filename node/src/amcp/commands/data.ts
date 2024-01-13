import path from "node:path";
import fs from "node:fs/promises";
import type { AMCPCommandEntry } from "../command_repository.js";
import { Native } from "../../native.js";

export function registerDataCommands(
    commands: Map<string, AMCPCommandEntry>,
    _channelCommands: Map<string, AMCPCommandEntry>
): void {
    // repo->register_command(L"Data Commands", L"DATA STORE", data_store_command, 2);
    // repo->register_command(L"Data Commands", L"DATA RETRIEVE", data_retrieve_command, 1);

    commands.set("DATA LIST", {
        func: async (context, command) => {
            let result = "200 DATA LIST OK\r\n";

            const subdirectory = command.parameters[0];
            let fullpath = context.configuration.paths.dataPath;
            if (subdirectory) fullpath = path.join(fullpath, subdirectory);

            const dirpath: string | null = await Native.FindCaseInsensitive(
                fullpath
            );
            if (!dirpath && subdirectory) {
                throw new Error(`Sub directory ${subdirectory} not found.`); // TODO file_not_found
            }

            if (dirpath) {
                const contents = await fs.readdir(dirpath, {
                    withFileTypes: true,
                });
                for (const fileinfo of contents) {
                    if (!fileinfo.isFile()) continue;
                    if (!fileinfo.name.endsWith(".ftd")) continue;

                    result += `${fileinfo.name
                        .slice(0, -4)
                        .toUpperCase()} \r\n`;
                }
            }

            return result + "\r\n";
        },
        minNumParams: 0,
    });

    commands.set("DATA REMOVE", {
        func: async (context, command) => {
            const rawpath = path.join(
                context.configuration.paths.dataPath,
                `${command.parameters[0]}.ftd`
            );

            const filename: string | null = await Native.FindCaseInsensitive(
                rawpath
            );
            if (!filename) throw new Error(`${rawpath} not found`); // TODO file_not_found

            try {
                await fs.unlink(filename);
            } catch (e: any) {
                if (e.code == "ENOENT") {
                    throw new Error(`${filename} not found`); // TODO file_not_found
                } else {
                    throw new Error(`${filename} could not be removed`); // TODO caspar_exception
                }
            }

            return "202 DATA REMOVE OK\r\n";
        },
        minNumParams: 1,
    });
}
