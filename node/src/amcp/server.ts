import { Server, Socket, createServer } from "net";
import { AMCPClientBatchInfo, type AMCPProtocolStrategy } from "./protocol.js";
import type { Client } from "./client.js";
import type { ChannelLocks } from "./channel_locks.js";

const LINE_DELIMITER = "\r\n";

export class AMCPServer {
    readonly #socket: Server;
    readonly #protocol: AMCPProtocolStrategy;
    readonly #locks: ChannelLocks;

    constructor(
        port: number,
        protocol: AMCPProtocolStrategy,
        locks: ChannelLocks
    ) {
        this.#protocol = protocol;
        this.#locks = locks;

        this.#socket = createServer(this.#onClient.bind(this));
        this.#socket.listen(port, () => {
            console.log(`AMCP listening on port ${port}`);
        });
    }

    #onClient(clientSocket: Socket): void {
        let address: string = (clientSocket.address() as any)?.address;
        if (address.startsWith("::ffff:")) address = address.slice(7);

        const client: Client = {
            id: "test",
            address: address,
            batch: new AMCPClientBatchInfo(),
        };

        console.log(`new client from ${client.address}`);

        clientSocket.on("close", () => {
            console.log(`lost client ${client.address}`);

            this.#locks.forgetClient(client);
        });

        let receiveBuffer = "";
        clientSocket.on("data", (data) => {
            receiveBuffer += data.toString();

            while (true) {
                const endIndex = receiveBuffer.indexOf(LINE_DELIMITER);
                if (endIndex === -1) break;

                const line = receiveBuffer.slice(0, endIndex).trim();
                receiveBuffer = receiveBuffer.slice(
                    endIndex + LINE_DELIMITER.length
                );

                if (line.toUpperCase() === "BYE") {
                    console.log(`Closing connection from ${client.address}`);
                    clientSocket.destroy();
                    return;
                }

                // TODO - does this need to go via a queue to limit concurrency?
                this.#protocol
                    .parse(client, line)
                    .then((response) => {
                        let responseStr =
                            response.join(LINE_DELIMITER) + LINE_DELIMITER;
                        if (responseStr.length == 2) return;

                        clientSocket.write(responseStr);

                        if (responseStr.length > 512) {
                            console.log(
                                `Sent ${responseStr.length} bytes to ${client.address}`
                            );
                        } else {
                            console.log(
                                `Sent message to ${client.address}: ${responseStr}`
                            );
                        }
                    })
                    .catch((e) => {
                        console.error(`AMCP error:`, e);
                    });
            }
        });
    }
}
