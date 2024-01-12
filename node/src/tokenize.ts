export function tokenize(message: string, pTokenVector: string[]): number {
    // split on whitespace but keep strings within quotationmarks
    // treat \ as the start of an escape-sequence: the following char will indicate what to actually put in the
    // string

    let currentToken: string = "";

    let inQuote = false;
    let inParamList = 0;
    let getSpecialCode = false;

    for (const messageChar of message) {
        if (getSpecialCode) {
            // insert code-handling here
            switch (messageChar) {
                case "\\":
                    currentToken += "\\";
                    break;
                case '"':
                    currentToken += '"';
                    break;
                case "n":
                    currentToken += "\n";
                    break;
                default:
                    break;
            }
            getSpecialCode = false;
            continue;
        }

        if (messageChar == "\\") {
            getSpecialCode = true;
            continue;
        }

        if (messageChar == " " && !inQuote && inParamList == 0) {
            if (currentToken.length > 0) {
                pTokenVector.push(currentToken);
                currentToken = "";
            }
            continue;
        } else if (!inQuote && messageChar == "(") {
            inParamList++;
            // continue;
        } else if (!inQuote && messageChar == ")") {
            inParamList--;
            if (inParamList == 0) {
                currentToken += messageChar;
                pTokenVector.push(currentToken);
                currentToken = "";
                continue;
            }
            // continue;
        } else if (messageChar == '"') {
            inQuote = !inQuote;

            if (inParamList == 0) {
                if (!inQuote) {
                    pTokenVector.push(currentToken);
                    currentToken = "";
                }
                continue;
            }
        }

        currentToken += messageChar;
    }

    if (currentToken.length > 0) {
        pTokenVector.push(currentToken);
        // currentToken = "";
    }

    return pTokenVector.length;
}
