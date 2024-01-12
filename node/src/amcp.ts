import { Native } from "./native.js";

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

export class AMCPProtocolStrategy {
    #parse_request_token(tokens: string[]): string | null {
        const first_token = tokens[0];
        if (!first_token || first_token.toUpperCase() !== "REQ") return null;

        const request_id = tokens[1];
        if (!request_id) return null;

        tokens.splice(0, 2);

        return request_id;
    }
}
