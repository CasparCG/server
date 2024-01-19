// @ts-ignore
import osc from "osc";

interface ClientInfo {
    address: string;
    port: number;
}

export class OscSender {
    readonly #osc: osc.UDPPort;

    #predefinedClients: Array<ClientInfo> = [];
    #ownedClients = new Map<string, ClientInfo[]>();

    constructor() {
        this.#osc = new osc.UDPPort({
            localAddress: "0.0.0.0",
            localPort: 0,
            broadcast: true,
            metadata: true,
        });
    }

    addOscPredefinedClient(address: string, port: number): boolean {
        // TODO - validate ip address?

        this.#predefinedClients.push({
            address,
            port,
        });

        return true;
    }

    addDestinationForOwner(
        ownerId: string,
        address: string,
        port: number
    ): boolean {
        // TODO - validate ip address?

        const ownedClients = this.#ownedClients.get(ownerId) ?? [];

        ownedClients.push({
            address,
            port,
        });

        this.#ownedClients.set(ownerId, ownedClients);

        return true;
    }

    removeDestinationsForOwner(ownerId: string): boolean {
        return this.#ownedClients.delete(ownerId);
    }

    sendState(channelId: number, state: unknown): void {
        // TODO
        console.log("send osc state", channelId, state, !!this.#osc);
    }
}
