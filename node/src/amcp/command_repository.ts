import type { OscSender } from "../osc.js";
import type { CasparCGConfiguration } from "../config/type.js";
import type { Client } from "./client.js";
import { registerDataCommands } from "./commands/data.js";
import { registerMediaScannerCommands } from "./commands/mediaScanner.js";
import { registerProducerCommands } from "./commands/producer.js";
import { registerSystemCommands } from "./commands/system.js";
import { registerConsumerCommands } from "./commands/consumer.js";
import { registerChannelCommands } from "./commands/channel.js";
import { registerMixerCommands } from "./commands/mixer.js";
import type { NativeTransform } from "../native.js";
import type { PartialDeep } from "type-fest";
import { isChannelIndexValid } from "./commands/util.js";

export interface AMCPCommandContext {
    configuration: CasparCGConfiguration;
    shutdown: (restart: boolean) => void;
    osc: OscSender;
    channelCount: number;
    channelStateStore: Map<number, Record<string, any[]>>;
    deferedTransforms: Map<number, TransformTuple[]>;
}

export interface TransformTuple {
    layerIndex: number;
    fragment: PartialDeep<NativeTransform>;
    duration: number;
    tween: string;
}

export type AMCPCommandCommitFunction = () => Promise<string>;

export type AMCPCommandFunction = (
    context: AMCPCommandContext,
    command: AMCPCommandBase
) => Promise<AMCPCommandCommitFunction | string>;

export type AMCPChannelCommandFunction = (
    context: AMCPCommandContext,
    command: AMCPCommandBase,
    channel: number,
    layer: number | null
) => Promise<AMCPCommandCommitFunction | string>;

export interface AMCPCommandEntry {
    func: AMCPCommandFunction;
    minNumParams: number;
}
export interface AMCPChannelCommandEntry {
    func: AMCPChannelCommandFunction;
    minNumParams: number;
}

export interface AMCPCommandBase {
    name: string;

    clientId: string;
    clientAddress: string;
    parameters: string[];
}

export interface NodeAMCPCommand extends AMCPCommandBase {
    execute: (
        context: AMCPCommandContext
    ) => Promise<AMCPCommandCommitFunction | string>;
}

export class AMCPCommandRepository {
    readonly #commands = new Map<string, AMCPCommandEntry>();
    readonly #channelCommands = new Map<string, AMCPChannelCommandEntry>();

    constructor() {
        registerChannelCommands(this.#commands, this.#channelCommands);
        registerProducerCommands(this.#commands, this.#channelCommands);
        registerConsumerCommands(this.#commands, this.#channelCommands);
        // TODO
        registerMixerCommands(this.#commands, this.#channelCommands);

        registerDataCommands(this.#commands, this.#channelCommands);
        registerMediaScannerCommands(this.#commands, this.#channelCommands);
        registerSystemCommands(this.#commands, this.#channelCommands);
    }

    #wrapChannelCommand(
        client: Client,
        name: string,
        channelIndex: number,
        layerIndex: number | null,
        tokens: string[],
        cmd: AMCPChannelCommandEntry
    ): NodeAMCPCommand | null {
        if (tokens.length < cmd.minNumParams) return null;

        const command: NodeAMCPCommand = {
            name,
            execute: async (context) => {
                if (!isChannelIndexValid(context, channelIndex)) {
                    // Ensure channel id is valid
                    return `401 ${name} ERROR\r\n`;
                }
                if (layerIndex !== null && layerIndex < 0) {
                    // Basic layer validation, but it can intentionally be null
                    return `401 ${name} ERROR\r\n`;
                }

                return cmd.func(context, command, channelIndex + 1, layerIndex);
            },
            clientId: client.id,
            clientAddress: client.address,
            parameters: tokens,
            // channelIndex,
            // layerIndex,
        };

        return command;
    }

    createChannelCommand(
        client: Client,
        name: string,
        channelIndex: number,
        layerIndex: number | null,
        tokens: string[]
    ): NodeAMCPCommand | null {
        // TODO - verify channel index

        const subcommand = tokens[0];
        if (subcommand) {
            const fullname = `${name} ${subcommand}`;
            const subcmd = this.#channelCommands.get(fullname.toUpperCase());
            if (subcmd) {
                tokens.shift();

                return this.#wrapChannelCommand(
                    client,
                    name,
                    channelIndex,
                    layerIndex,
                    tokens,
                    subcmd
                );
            }
        }

        // Resort to ordinary command
        const command = this.#channelCommands.get(name.toUpperCase());

        if (command) {
            return this.#wrapChannelCommand(
                client,
                name,
                channelIndex,
                layerIndex,
                tokens,
                command
            );
        }

        return null;
    }

    #wrapBareCommand(
        client: Client,
        name: string,
        tokens: string[],
        cmd: AMCPCommandEntry
    ): NodeAMCPCommand | null {
        if (tokens.length < cmd.minNumParams) return null;

        const command: NodeAMCPCommand = {
            name,
            execute: (context) => cmd.func(context, command),
            clientId: client.id,
            clientAddress: client.address,
            parameters: tokens,
        };

        return command;
    }

    createCommand(
        client: Client,
        name: string,
        tokens: string[]
    ): NodeAMCPCommand | null {
        const subcommand = tokens[0];
        if (subcommand) {
            const fullname = `${name} ${subcommand}`;
            const subcmd = this.#commands.get(fullname.toUpperCase());
            if (subcmd) {
                tokens.shift();

                return this.#wrapBareCommand(client, name, tokens, subcmd);
            }
        }

        // Resort to ordinary command
        const command = this.#commands.get(name.toUpperCase());

        if (command) {
            return this.#wrapBareCommand(client, name, tokens, command);
        }

        return null;
    }
}
