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

export interface AMCPCommandContext {
    configuration: CasparCGConfiguration;
    shutdown: (restart: boolean) => void;
    osc: OscSender;
    channelCount: number;
    deferedTransforms: Map<number, TransformTuple[]>;
}

export interface TransformTuple {
    layerIndex: number;
    fragment: PartialDeep<NativeTransform>;
    duration: number;
    tween: string;
}

export type AMCPCommandFunction = (
    context: AMCPCommandContext,
    command: AMCPCommandBase
) => Promise<string>;

export interface AMCPCommandEntry {
    func: AMCPCommandFunction;
    minNumParams: number;
}

export interface AMCPCommandBase {
    name: string;

    clientId: string;
    clientAddress: string;
    channelIndex: number | null;
    layerIndex: number | null;
    parameters: string[];
}

export interface NodeAMCPCommand extends AMCPCommandBase {
    command: AMCPCommandFunction;
}

function make_cmd(
    cmd: AMCPCommandEntry,
    name: string,
    client: Client,
    channelIndex: number | null,
    layerIndex: number | null,
    tokens: string[]
): NodeAMCPCommand | null {
    if (tokens.length < cmd.minNumParams) return null;

    return {
        name,
        command: cmd.func,
        clientId: client.id,
        clientAddress: client.address,
        channelIndex,
        layerIndex,
        parameters: tokens,
    };
}

export class AMCPCommandRepository {
    readonly #commands = new Map<string, AMCPCommandEntry>();
    readonly #channelCommands = new Map<string, AMCPCommandEntry>();

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

    createChannelCommand(
        client: Client,
        name: string,
        channelIndex: number,
        layerIndex: number | null,
        tokens: string[]
    ): NodeAMCPCommand | null {
        // TODO - verify channel index

        return this.#findCommand(
            this.#channelCommands,
            client,
            name,
            channelIndex,
            layerIndex,
            tokens
        );
    }

    createCommand(
        client: Client,
        name: string,
        tokens: string[]
    ): NodeAMCPCommand | null {
        return this.#findCommand(
            this.#commands,
            client,
            name,
            null,
            null,
            tokens
        );
    }

    #findCommand(
        commands: ReadonlyMap<string, AMCPCommandEntry>,
        client: Client,
        name: string,
        channelIndex: number | null,
        layerIndex: number | null,
        tokens: string[]
    ): NodeAMCPCommand | null {
        const subcommand = tokens[0];
        if (subcommand) {
            const fullname = `${name} ${subcommand}`;
            const subcmd = commands.get(fullname.toUpperCase());
            if (subcmd) {
                tokens.shift();

                return make_cmd(
                    subcmd,
                    fullname,
                    client,
                    channelIndex,
                    layerIndex,
                    tokens
                );
            }
        }

        // Resort to ordinary command
        const command = commands.get(name.toUpperCase());

        if (command) {
            return make_cmd(
                command,
                name,
                client,
                channelIndex,
                layerIndex,
                tokens
            );
        }

        return null;
    }
}
