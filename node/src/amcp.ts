import { Native } from "./native.js";
import { tokenize } from "./tokenize.js";

export type AMCPCommand = unknown;

export interface AMCPGroupCommand {
    readonly requestId: string | null;
    readonly commands: AMCPCommand[];
}

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

    finish(): AMCPGroupCommand {
        this.#inProgress = false;

        // commands_.clear();

        return {
            requestId: this.#requestId,
            commands: this.#commands,
        };
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
    parse(message: string, batch: AMCPClientBatchInfo): string {
        const tokens = tokenize(message);

        if (tokens.length !== 0 && tokens[0].toUpperCase() === "PING") {
            tokens.pop();

            const answer = ["PONG", ...tokens];

            return answer.join(" ");
        }

        console.log(`Received message from TODO: ${message}`);

        const result = this.#parse_command_string(batch, tokens);
        if (result.status !== AMCPCommandError.no_error) {
            //
        } else {
            // TODO: this needs porting
            return "";
        }
        // std::wstring request_id;
        // std::wstring command_name;
        // error_state  err = parse_command_string(client, batch, tokens, request_id, command_name);
        // if (err != error_state::no_error) {
        //     std::wstringstream answer;

        //     if (!request_id.empty())
        //         answer << L"RES " << request_id << L" ";

        //     switch (err) {
        //         case error_state::command_error:
        //             answer << L"400 ERROR\r\n" << message << "\r\n";
        //             break;
        //         case error_state::channel_error:
        //             answer << L"401 " << command_name << " ERROR\r\n";
        //             break;
        //         case error_state::parameters_error:
        //             answer << L"402 " << command_name << " ERROR\r\n";
        //             break;
        //         case error_state::access_error:
        //             answer << L"503 " << command_name << " FAILED\r\n";
        //             break;
        //         case error_state::unknown_error:
        //             answer << L"500 FAILED\r\n";
        //             break;
        //         default:
        //             CASPAR_THROW_EXCEPTION(programming_error() << msg_info(L"Unhandled error_state enum constant " +
        //                                                                    std::to_wstring(static_cast<int>(err))));
        //     }
        //     client->send(answer.str());
        // }
        return "";
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

            commandName = tokens[0].toLocaleUpperCase();
            const command = new Native.AMCPCommand(tokens);
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
