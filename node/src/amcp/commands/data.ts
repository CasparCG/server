import path from "node:path";
import fs from "node:fs/promises";
import type {
    AMCPChannelCommandEntry,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";

export async function read_data_file(filepath: string): Promise<string> {
    try {
        const contents = await fs.readFile(filepath);

        if (contents[0] == 0xef && contents[1] == 0xbb && contents[2] == 0xbf) {
            return contents.subarray(3).toString("utf8");
        } else {
            return contents.toString("latin1");
        }
    } catch (e) {
        // TODO: better error handling?
        throw new Error(`${filepath} not found`); // TODO file_not_found
    }
}

export function registerDataCommands(
    commands: Map<string, AMCPCommandEntry>,
    _channelCommands: Map<string, AMCPChannelCommandEntry>
): void {
    commands.set("DATA STORE", {
        func: async (context, command) => {
            const rawpath = path.join(
                context.configuration.paths.dataPath,
                `${command.parameters[0]}.ftd`
            );

            // Ensure the containing directory exists
            const parentPath = path.dirname(rawpath);
            const parentPath2: string | null = await Native.FindCaseInsensitive(
                parentPath
            );
            if (!parentPath2) {
                await fs.mkdir(parentPath, { recursive: true });
            }

            let writePath = path.join(
                parentPath2 ?? parentPath,
                path.basename(rawpath)
            );
            writePath =
                (await Native.FindCaseInsensitive(writePath)) ?? writePath;

            const fileData = command.parameters[1];
            const bufferLength = Buffer.byteLength(fileData, "utf8");
            const writeBuffer = Buffer.alloc(bufferLength + 2);

            // Write UTF8 BOM
            writeBuffer.writeUint8(0xef, 0);
            writeBuffer.writeUint8(0xbb, 1);
            writeBuffer.writeUint8(0xbf, 2);

            writeBuffer.write(fileData, 3, "utf8");

            await fs.writeFile(writePath, writeBuffer);

            return "202 DATA STORE OK\r\n";
        },
        minNumParams: 2,
    });

    commands.set("DATA RETRIEVE", {
        func: async (context, command) => {
            const rawpath = path.join(
                context.configuration.paths.dataPath,
                `${command.parameters[0]}.ftd`
            );

            const filepath: string | null = await Native.FindCaseInsensitive(
                rawpath
            );
            if (!filepath) throw new Error(`${rawpath} not found`); // TODO file_not_found

            const contents = await read_data_file(filepath);

            return `201 DATA RETRIEVE OK\r\n${contents}\r\n`;
        },
        minNumParams: 1,
    });

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
