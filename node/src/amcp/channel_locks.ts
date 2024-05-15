import type { Client } from "./client.js";

interface ChannelLock {
    clientIds: Set<string>;
    lockPhrase: string;
}

export class ChannelLocks {
    readonly #locked = new Map<number, ChannelLock>();
    readonly #overridePhrase: string;

    constructor(overridePhrase: string) {
        this.#overridePhrase = overridePhrase;
    }

    public forgetClient(client: Client): void {
        for (const key of this.#locked.keys()) {
            this.releaseLock(client, key);
        }
    }

    public isChannelLocked(client: Client, channelIndex: number): boolean {
        if (channelIndex < 0) return false;

        const lock = this.#locked.get(channelIndex);
        if (!lock) return false;
        return !lock.clientIds.has(client.id);
    }

    public releaseLock(client: Client, channelIndex: number): void {
        const lock = this.#locked.get(channelIndex);
        if (!lock) return;

        if (lock.clientIds.delete(client.id)) {
            console.info(`Channel ${channelIndex} released`);

            if (lock.clientIds.size === 0) {
                this.#locked.delete(channelIndex);
            }
        }
    }

    public tryLock(
        client: Client,
        channelIndex: number,
        lockPhrase: string
    ): boolean {
        const lock = this.#locked.get(channelIndex) ?? {
            clientIds: new Set(),
            lockPhrase: "",
        };
        this.#locked.set(channelIndex, lock);

        if (lock.lockPhrase && lock.lockPhrase !== lockPhrase) {
            return false;
        }

        lock.lockPhrase = lockPhrase;
        lock.clientIds.add(client.id);

        console.info(`Channel ${channelIndex} acquired`);

        return true;
    }

    public clearLocks(overridePhrase: string | undefined): boolean {
        if (this.#overridePhrase && this.#overridePhrase !== overridePhrase) {
            return false;
        }

        this.#locked.clear();

        console.info("Channel locks cleared");
        return true;
    }
}
