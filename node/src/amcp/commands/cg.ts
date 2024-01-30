import type {
    AMCPChannelCommandEntry,
    AMCPCommandEntry,
} from "../command_repository.js";
import { Native } from "../../native.js";
import { discardError } from "./util.js";
import { read_data_file } from "./data.js";
import path from "path";

const CG_DEFAULT_LAYER = 9999;

export function registerProducerCommands(
    _commands: Map<string, AMCPCommandEntry>,
    channelCommands: Map<string, AMCPChannelCommandEntry>
): void {
    // repo->register_channel_command(L"Template Commands", L"CG ADD", cg_add_command, 3);

    channelCommands.set("CG ADD", {
        func: async (context, command, channelIndex, layerIndex0) => {
            const layerIndex = layerIndex0 ?? CG_DEFAULT_LAYER;

            // CG 1 ADD 0 "template_folder/templatename" [STARTLABEL] 0/1 [DATA]

            const layer = Number(command.parameters[0]);
            let label = ""; //_parameters[2]
            let bDoStart = false; //_parameters[2] alt. _parameters[3]
            let dataIndex = 3;

            if (command.parameters[2]) {
                // read label
                label = command.parameters[2];
                ++dataIndex;

                if (command.parameters[3])
                    // read play-on-load-flag
                    bDoStart = command.parameters[3] == "1";
            } else {
                // read play-on-load-flag
                bDoStart = command.parameters[2] == "1";
            }

            let pDataString: string | null = null;
            if (command.parameters.length > dataIndex) {
                // read data
                const dataString = command.parameters[dataIndex];

                if (dataString.startsWith("<") || dataString.startsWith("{")) {
                    // the data is XML or Json
                    pDataString = dataString;
                } else {
                    // The data is not an XML-string, it must be a filename
                    const rawpath = path.join(
                        context.configuration.paths.dataPath,
                        `${dataString}.ftd`
                    );

                    const filepath: string | null =
                        await Native.FindCaseInsensitive(rawpath);
                    if (!filepath) throw new Error(`${rawpath} not found`); // TODO file_not_found

                    pDataString = await read_data_file(filepath);
                }
            }

            const filename = command.parameters[1];
            return async () => {
                discardError(
                    Native.CallCgMethod(
                        "add",
                        channelIndex,
                        layerIndex,
                        layer,
                        filename,
                        bDoStart,
                        label,
                        pDataString ?? ""
                    )
                );

                return "202 CG OK\r\n";
            };
        },
        minNumParams: 3,
    });

    channelCommands.set("CG PLAY", {
        func:
            async (_context, command, channelIndex, layerIndex) => async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                const cgLayer = Number(command.parameters[0]);

                discardError(
                    Native.CallCgMethod(
                        "play",
                        channelIndex,
                        layerIndex,
                        cgLayer
                    )
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 1,
    });

    channelCommands.set("CG STOP", {
        func:
            async (_context, command, channelIndex, layerIndex) => async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                const cgLayer = Number(command.parameters[0]);

                discardError(
                    Native.CallCgMethod(
                        "stop",
                        channelIndex,
                        layerIndex,
                        cgLayer
                    )
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 1,
    });

    channelCommands.set("CG NEXT", {
        func:
            async (_context, command, channelIndex, layerIndex) => async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                const cgLayer = Number(command.parameters[0]);

                discardError(
                    Native.CallCgMethod(
                        "next",
                        channelIndex,
                        layerIndex,
                        cgLayer
                    )
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 1,
    });

    channelCommands.set("CG REMOVE", {
        func:
            async (_context, command, channelIndex, layerIndex) => async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                const cgLayer = Number(command.parameters[0]);

                discardError(
                    Native.CallCgMethod(
                        "remove",
                        channelIndex,
                        layerIndex,
                        cgLayer
                    )
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 1,
    });

    channelCommands.set("CG CLEAR", {
        func:
            async (_context, _command, channelIndex, layerIndex) =>
            async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                discardError(
                    Native.CallStageMethod("clear", channelIndex, layerIndex)
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 0,
    });

    channelCommands.set("CG UPDATE", {
        func: async (context, command, channelIndex, layerIndex0) => {
            const layerIndex = layerIndex0 ?? CG_DEFAULT_LAYER;

            const cgLayer = Number(command.parameters[0]);
            let payload = command.parameters[1];

            if (!payload.startsWith("<") && !payload.startsWith("{")) {
                // The data is not XML or Json, it must be a filename
                const rawpath = path.join(
                    context.configuration.paths.dataPath,
                    `${payload}.ftd`
                );

                const filepath: string | null =
                    await Native.FindCaseInsensitive(rawpath);
                if (!filepath) throw new Error(`${rawpath} not found`); // TODO file_not_found

                payload = await read_data_file(filepath);
            }

            return async () => {
                discardError(
                    Native.CallCgMethod(
                        "update",
                        channelIndex,
                        layerIndex,
                        cgLayer,
                        payload
                    )
                );

                return "202 CG OK\r\n";
            };
        },
        minNumParams: 2,
    });

    channelCommands.set("CG INVOKE", {
        func:
            async (_context, command, channelIndex, layerIndex) => async () => {
                layerIndex = layerIndex ?? CG_DEFAULT_LAYER;

                const cgLayer = Number(command.parameters[0]);
                const invoke = command.parameters[1];

                discardError(
                    Native.CallCgMethod(
                        "invoke",
                        channelIndex,
                        layerIndex,
                        cgLayer,
                        invoke
                    )
                );

                return "202 CG OK\r\n";
            },
        minNumParams: 2,
    });
}
