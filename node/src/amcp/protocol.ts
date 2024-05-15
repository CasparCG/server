import { tokenize } from "./tokenize.js";
import type { ChannelLocks } from "./channel_locks.js";
import type { Client } from "./client.js";
import type {
    NodeAMCPCommand,
    AMCPCommandContext,
    AMCPCommandRepository,
} from "./command_repository.js";

export class AMCPClientBatchInfo {
    readonly #commands: NodeAMCPCommand[] = [];
    #inProgress = false;
    #requestId: string | null = null;

    get requestId(): string | null {
        return this.#requestId;
    }

    get inProgress(): boolean {
        return this.#inProgress;
    }

    addCommand(cmd: NodeAMCPCommand): void {
        this.#commands.push(cmd);
    }

    begin(requestId: string | null): void {
        this.#requestId = requestId;
        this.#inProgress = true;
        this.#commands.length = 0;
    }

    finish(): NodeAMCPCommand[] {
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
    readonly #commandRepository: AMCPCommandRepository;
    readonly #locks: ChannelLocks;
    readonly #context: AMCPCommandContext;

    constructor(
        commandRepository: AMCPCommandRepository,
        locks: ChannelLocks,
        context: AMCPCommandContext
    ) {
        this.#commandRepository = commandRepository;
        this.#locks = locks;
        this.#context = context;
    }

    async parse(client: Client, message: string): Promise<string[]> {
        const tokens = tokenize(message);

        if (tokens.length === 0) {
            // Ignore empty
            return [];
        }

        if (tokens[0].toUpperCase() === "PING") {
            tokens.pop();

            const answer = ["PONG", ...tokens];

            return [answer.join(" ")];
        }

        console.log(`Received message from ${client.address}: ${message}`);

        const result = await this.#parse_command_string(client, tokens);
        if (Array.isArray(result)) {
            return result;
        } else if (result.status !== AMCPCommandError.no_error) {
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
                    return [
                        result.commandName
                            ? `500 ${result.commandName} FAILED`
                            : "500 FAILED",
                    ];
                default:
                    throw new Error(
                        `Unhandled error_state enum constant ${result.status}`
                    );
            }
        } else {
            return result.lines ?? [];
        }
    }

    async #parse_command_string(
        client: Client,
        tokens: string[]
    ): Promise<
        | {
              status: AMCPCommandError;
              requestId: string | null;
              commandName: string | undefined;
              lines?: string[];
          }
        | string[]
    > {
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

            const batchResponse = await this.#parse_batch_commands(
                client.batch,
                tokens,
                requestId
            );
            if (batchResponse !== null) {
                return batchResponse;
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

            if (commandName === "LOCK") {
                return this.#parse_lock_command(client, tokens);
            }

            const channelIds = this.#parse_channel_id(tokens);

            let command: NodeAMCPCommand | null | undefined;
            if (channelIds) {
                command = this.#commandRepository.createChannelCommand(
                    client,
                    commandName,
                    channelIds.channelIndex,
                    channelIds.layerIndex ?? null,
                    tokens
                );
            }
            if (!command) {
                command = this.#commandRepository.createCommand(
                    client,
                    commandName,
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

            if (
                channelIds &&
                this.#locks.isChannelLocked(client, channelIds.channelIndex)
            ) {
                return {
                    status: AMCPCommandError.access_error,
                    requestId,
                    commandName,
                };
            }

            let lines: string[] | undefined;

            if (client.batch.inProgress) {
                client.batch.addCommand(command);
            } else {
                let line = await command.execute(this.#context);
                if (typeof line === "function") line = await line(); // Perform the commit
                lines = [line];
            }

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
                lines,
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

    async #parse_batch_commands(
        batch: AMCPClientBatchInfo,
        tokens: string[],
        requestId: string | null
    ): Promise<{
        status: AMCPCommandError;
        requestId: string | null;
        commandName: string | undefined;
        lines?: string[];
    } | null> {
        const commandName = tokens[0].toLocaleUpperCase();
        if (commandName == "COMMIT") {
            if (!batch.inProgress) {
                return {
                    status: AMCPCommandError.command_error,
                    requestId,
                    commandName,
                };
            }

            let lines: string[] = [];

            // TODO: this is probably not atomic enough, and could have the wrong order?
            for (const command of batch.finish()) {
                let line = await command.execute(this.#context);
                if (typeof line === "function") line = await line(); // TODO - batch this correctly
                lines.push();
            }

            return {
                status: AMCPCommandError.no_error,
                requestId,
                commandName,
                lines,
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

    #parse_lock_command(
        client: Client,
        tokens: string[]
    ):
        | {
              status: AMCPCommandError;
              requestId: string | null;
              commandName: string | undefined;
          }
        | string[] {
        const channelIndex = Number(tokens[0]) - 1;
        if (isNaN(channelIndex)) {
            throw new Error(`Invalid channel index`);
        }

        const command = tokens[1].toLocaleUpperCase();
        const lockPhrase = tokens[2] as string | undefined;

        if (command == "ACQUIRE") {
            // TODO: read options

            // just lock one channel
            if (
                !lockPhrase ||
                !this.#locks.tryLock(client, channelIndex, lockPhrase)
            )
                return ["503 LOCK ACQUIRE FAILED"];

            return ["202 LOCK ACQUIRE OK"];
        }

        if (command == "RELEASE") {
            this.#locks.releaseLock(client, channelIndex);

            return ["202 LOCK RELEASE OK"];
        }

        if (command == "CLEAR") {
            if (!this.#locks.clearLocks(lockPhrase))
                return ["503 LOCK CLEAR FAILED"];

            return ["202 LOCK CLEAR OK"];
        }

        throw new Error(`Unknown LOCK command ${command}`);
    }
}
