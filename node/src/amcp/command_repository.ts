import type { OscSender } from "../osc.js";
import type { CasparCGConfiguration } from "../config/type.js";
import type { Client } from "./client.js";
import { registerDataCommands } from "./commands/data.js";
import { registerMediaScannerCommands } from "./commands/mediaScanner.js";
import { registerProducerCommands } from "./commands/producer.js";
import { registerSystemCommands } from "./commands/system.js";

export interface AMCPCommandContext {
    configuration: CasparCGConfiguration;
    shutdown: (restart: boolean) => void;
    osc: OscSender;
    channelCount: number;
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

export interface AMCPCommand2 extends AMCPCommandBase {
    command: AMCPCommandFunction;
}

function make_cmd(
    cmd: AMCPCommandEntry,
    name: string,
    client: Client,
    channelIndex: number | null,
    layerIndex: number | null,
    tokens: string[]
): AMCPCommand2 | null {
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
        registerProducerCommands(this.#commands, this.#channelCommands);
        // TODO

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
    ): AMCPCommand2 | null {
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
    ): AMCPCommand2 | null {
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
    ): AMCPCommand2 | null {
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
