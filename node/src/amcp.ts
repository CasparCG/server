import { Native } from "./native.js";
import { tokenize } from "./tokenize.js";

export type AMCPCommand = unknown;

export class AMCPClientBatchInfo {
    readonly #commands: AMCPCommand[] = [];
    #inProgress = false;
    #requestId: string | null = null;

    get requestId(): string | null {
        return this.#requestId;
    }

    get inProgress(): boolean {
        return this.#inProgress;
    }

    addCommand(cmd: AMCPCommand): void {
        this.#commands.push(cmd);
    }

    begin(requestId: string | null): void {
        this.#requestId = requestId;
        this.#inProgress = true;
        this.#commands.length = 0;
    }

    finish(): AMCPCommand[] {
        this.#inProgress = false;

        // commands_.clear();

        return this.#commands;
    }
}

enum AMCPCommandError {
    no_error = 0,
    command_error,
    channel_error,
    parameters_error,
    unknown_error,
    access_error,
}

export class AMCPProtocolStrategy {
    parse(message: string, batch: AMCPClientBatchInfo): string[] {
        const tokens = tokenize(message);

        if (tokens.length !== 0 && tokens[0].toUpperCase() === "PING") {
            tokens.pop();

            const answer = ["PONG", ...tokens];

            return [answer.join(" ")];
        }

        console.log(`Received message from TODO: ${message}`);

        const result = this.#parse_command_string(batch, tokens);
        if (result.status !== AMCPCommandError.no_error) {
            switch (result.status) {
                case AMCPCommandError.command_error:
                    return ["400 ERROR", message];
                case AMCPCommandError.channel_error:
                    return [`401 ${result.commandName} ERROR`];
                case AMCPCommandError.parameters_error:
                    return [`402 ${result.commandName} ERROR`];
                case AMCPCommandError.access_error:
                    return [`503 ${result.commandName} FAILED`];
                case AMCPCommandError.unknown_error:
                    return ["500 FAILED"];
                default:
                    throw new Error(
                        `Unhandled error_state enum constant ${result.status}`
                    );
            }
        } else {
            // TODO: this needs porting
            return [];
        }
    }

    #parse_command_string(
        batch: AMCPClientBatchInfo,
        tokens: string[]
    ): {
        status: AMCPCommandError;
        requestId: string | null;
        commandName: string | undefined;
    } {
        let requestId: string | null = null;
        let commandName: string | undefined;

        try {
            // Discard GetSwitch
            if (tokens.length !== 0 && tokens[0].startsWith("/"))
                tokens.shift();

            const requestId = this.#parse_request_token(tokens);

            // Fail if no more tokens.
            if (tokens.length === 0) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName: undefined,
                };
            }

            const batchCommand = this.#parse_batch_commands(
                batch,
                tokens,
                requestId
            );
            if (batchCommand !== null) {
                return batchCommand;
            }

            // Fail if no more tokens.
            if (tokens.length === 0) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName: undefined,
                };
            }

            commandName = tokens.shift()!.toLocaleUpperCase();
            const channelIds = this.#parse_channel_id(tokens);

            let command: AMCPCommand | undefined;

            if (channelIds) {
                command = new Native.AMCPCommand(
                    commandName,
                    channelIds.channelIndex ?? -1,
                    channelIds.layerIndex ?? -1,
                    tokens
                );
            }
            if (!command) {
                command = new Native.AMCPCommand(
                    commandName,
                    -1,
                    -1,
                    channelIds ? [channelIds.channelSpec, ...tokens] : tokens
                );
            }

            if (!command) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName,
                };
            }

            // auto channel_index = command.channel_index();
            // if (!repo_->check_channel_lock(client, channel_index)) {
            //     return error_state::access_error;
            // }

            if (batch.inProgress) {
                batch.addCommand(command);
            } else {
                // TODO: correct queue
                Native.executeCommandBatch([command]);
            }

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
            };
            // } catch (std::out_of_range&) {
            //     CASPAR_LOG(error) << "Invalid channel specified.";
            //     return error_state::channel_error;
        } catch (e) {
            console.log(e);

            return {
                status: AMCPCommandError.unknown_error,
                requestId,
                commandName,
            };
        }
    }

    #parse_channel_id(tokens: string[]): {
        channelSpec: string;
        channelIndex: number;
        layerIndex: number | undefined;
    } | null {
        if (tokens.length === 0) return null;

        const channelSpec = tokens[0];
        const ids = channelSpec.split("-");

        const channelIndex = Number(ids[0]) - 1;

        if (!isNaN(channelIndex)) {
            // Consume channel-spec
            tokens.shift();

            const layerIndex = ids.length > 1 ? Number(ids[1]) : undefined;

            return {
                channelSpec,
                channelIndex,
                layerIndex,
            };
        } else {
            return null;
        }
    }

    #parse_request_token(tokens: string[]): string | null {
        const first_token = tokens[0];
        if (!first_token || first_token.toUpperCase() !== "REQ") return null;

        const request_id = tokens[1];
        if (!request_id) return null;

        tokens.splice(0, 2);

        return request_id;
    }

    #parse_batch_commands(
        batch: AMCPClientBatchInfo,
        tokens: string[],
        requestId: string | null
    ): {
        status: AMCPCommandError;
        requestId: string | null;
        commandName: string | undefined;
    } | null {
        const commandName = tokens[0].toLocaleUpperCase();
        if (commandName == "COMMIT") {
            if (!batch.inProgress) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName,
                };
            }

            // TODO: correct queue
            Native.executeCommandBatch(batch.finish(), batch.requestId);

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
            };
        }

        if (commandName == "BEGIN") {
            if (batch.inProgress) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName,
                };
            }

            batch.begin(requestId);

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
            };
        }

        if (commandName == "DISCARD") {
            if (!batch.inProgress) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName,
                };
            }

            batch.finish();

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
            };
        }

        return null;
    }
}
