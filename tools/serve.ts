import {
    createServer,
    type IncomingMessage,
    type ServerResponse,
} from "node:http";
import { readFile, stat } from "node:fs/promises";
import { extname, join, normalize } from "node:path";

const defaultRoot = join(
    process.env.HOME ?? ".",
    "Developer",
    "dawn",
    "out",
    "mira-debug",
    "wasm",
);
const root = process.env.MIRA_WEB_ROOT ?? defaultRoot;
const host = process.env.MIRA_WEB_HOST ?? "127.0.0.1";
const port = Number(process.env.MIRA_WEB_PORT ?? "4173");
const index = "mira_web.html";

const mime = (path: string): string => {
    switch (extname(path)) {
        case ".html":
            return "text/html; charset=utf-8";
        case ".js":
            return "text/javascript; charset=utf-8";
        case ".css":
            return "text/css; charset=utf-8";
        case ".wasm":
            return "application/wasm";
        default:
            return "application/octet-stream";
    }
};

const resolvePath = (pathname: string): string | null => {
    let clean: string;
    try {
        clean = decodeURIComponent(pathname).replace(/^\/+/, "");
    } catch {
        return null;
    }
    const candidate = normalize(join(root, clean));
    const normalizedRoot = normalize(root);
    if (
        candidate !== normalizedRoot &&
        !candidate.startsWith(`${normalizedRoot}/`)
    ) {
        return null;
    }
    return candidate;
};

const readStaticFile = async (
    pathname: string,
): Promise<{ body: Uint8Array; type: string } | null> => {
    const file = resolvePath(pathname);
    if (file === null) {
        return null;
    }

    try {
        const info = await stat(file);
        if (!info.isFile()) {
            return null;
        }
        return { body: await readFile(file), type: mime(file) };
    } catch {
        return null;
    }
};

const server = createServer(
    async (request: IncomingMessage, response: ServerResponse) => {
        let url: URL;
        try {
            url = new URL(request.url ?? "/", `http://${host}:${port}`);
        } catch {
            response.writeHead(400);
            response.end();
            return;
        }
        const asset = await readStaticFile(url.pathname);
        const shouldFallback = asset === null && extname(url.pathname) === "";
        const file =
            asset ??
            (shouldFallback ? await readStaticFile(`/${index}`) : null);

        if (file === null) {
            response.writeHead(404);
            response.end();
            return;
        }

        response.writeHead(200, { "content-type": file.type });
        response.end(file.body);
    },
);

server.listen(port, host, () => {
    console.log(`http://${host}:${port}/`);
});
