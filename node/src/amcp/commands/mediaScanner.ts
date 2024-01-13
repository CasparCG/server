import type {
    AMCPCommandContext,
    AMCPCommandEntry,
} from "../command_repository.js";

async function makeRequest(
    context: AMCPCommandContext,
    path: string,
    errorResponse: string
): Promise<string> {
    const { mediaScanner } = context.configuration;
    try {
        const req = await fetch(
            `http://${mediaScanner.host}:${mediaScanner.port}${path}`,
            {
                signal: AbortSignal.timeout(5000),
            }
        );

        const text = await req.text();
        return text || errorResponse;
    } catch (e) {
        console.log("Media scanner failed", e);
        return errorResponse;
    }
}

export function registerMediaScannerCommands(
    commands: Map<string, AMCPCommandEntry>,
    _channelCommands: Map<string, AMCPCommandEntry>
): void {
    commands.set("THUMBNAIL LIST", {
        func: async (context) => {
            return makeRequest(
                context,
                "/thumbnail",
                "501 THUMBNAIL LIST FAILED\r\n"
            );
        },
        minNumParams: 0,
    });

    commands.set("THUMBNAIL RETRIEVE", {
        func: async (context, command) => {
            return makeRequest(
                context,
                `/thumbnail/${encodeURIComponent(command.parameters[0])}`,
                "501 THUMBNAIL RETRIEVE FAILED\r\n"
            );
        },
        minNumParams: 1,
    });

    commands.set("THUMBNAIL GENERATE", {
        func: async (context, command) => {
            return makeRequest(
                context,
                `/thumbnail/generate/${encodeURIComponent(
                    command.parameters[0]
                )}`,
                "501 THUMBNAIL GENERATE FAILED\r\n"
            );
        },
        minNumParams: 1,
    });

    commands.set("THUMBNAIL GENERATE_ALL", {
        func: async (context) => {
            return makeRequest(
                context,
                `/thumbnail/generate`,
                "501 THUMBNAIL GENERATE_ALL FAILED\r\n"
            );
        },
        minNumParams: 0,
    });

    commands.set("CINF", {
        func: async (context, command) => {
            return makeRequest(
                context,
                `/cinf/${encodeURIComponent(command.parameters[0])}`,
                "501 CINF FAILED\r\n"
            );
        },
        minNumParams: 1,
    });

    commands.set("CLS", {
        func: async (context) => {
            return makeRequest(context, `/cls`, "501 CLS FAILED\r\n");
        },
        minNumParams: 0,
    });

    commands.set("FLS", {
        func: async (context) => {
            return makeRequest(context, `/fls`, "501 FLS FAILED\r\n");
        },
        minNumParams: 0,
    });

    commands.set("TLS", {
        func: async (context) => {
            return makeRequest(context, `/tls`, "501 TLS FAILED\r\n");
        },
        minNumParams: 0,
    });
}
