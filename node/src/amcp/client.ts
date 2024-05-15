import type { AMCPClientBatchInfo } from "./protocol.js";

export interface Client {
    readonly id: string;
    readonly address: string;

    readonly batch: AMCPClientBatchInfo;
}
