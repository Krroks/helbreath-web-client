// Dev server for the browser client: / -> Web/out, /data/ -> the game folder (with HTTP Range, the client reads
// .pak files in pieces). Local only.
// usage: node serve.mjs [port=8080] [gameDir=G:/DEV/hb/run/client] [bridgePort=8000]
import http from "node:http";
import net from "node:net";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const port = Number(process.argv[2] || 8080);
const gameDir = path.resolve(process.argv[3] || "G:/DEV/hb/run/client");
const bridgePort = Number(process.argv[4] || 8000);
const outDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../out");
const types = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".wasm": "application/wasm" };

// Resolves a URL path inside root; null if it would escape it.
function inside(root, urlPath) {
  const p = path.resolve(root, "." + decodeURIComponent(urlPath));
  return p === root || p.startsWith(root + path.sep) ? p : null;
}

// Hot reload: pages listening on /events (server-sent events) get "sprites/<name>.pak" when a pak in SPRITES changes.
const listeners = new Set();
const pending = new Map();
const spritesDir = fs.readdirSync(gameDir).find((n) => n.toLowerCase() === "sprites");
if (spritesDir) fs.watch(path.join(gameDir, spritesDir), (_, name) => {
  if (!name || !/\.pak$/i.test(name)) return;
  clearTimeout(pending.get(name)); // writers save in several steps: announce once it is quiet
  pending.set(name, setTimeout(() => {
    pending.delete(name);
    console.log(`changed: ${name} -> ${listeners.size} page(s)`);
    for (const res of listeners) res.write(`data: sprites/${name}\n\n`);
  }, 400));
});

const server = http.createServer((req, res) => {
  const url = new URL(req.url, "http://x");
  if (url.pathname === "/events") {
    res.writeHead(200, { "Content-Type": "text/event-stream", "Cache-Control": "no-cache" });
    res.write(": hot reload\n\n");
    listeners.add(res);
    req.on("close", () => listeners.delete(res));
    return;
  }
  const file = url.pathname.startsWith("/data/")
    ? inside(gameDir, url.pathname.slice(5))
    : inside(outDir, url.pathname === "/" ? "/index.html" : url.pathname);
  let st;
  try { st = file && fs.statSync(file); } catch { st = null; }
  if (!st || !st.isFile()) { res.writeHead(404).end(); return; }
  const headers = { "Content-Type": types[path.extname(file)] || "application/octet-stream", "Accept-Ranges": "bytes", "Cache-Control": "no-cache" };
  const m = /^bytes=(\d+)-(\d*)$/.exec(req.headers.range || "");
  if (m) {
    const start = Number(m[1]), end = Math.min(m[2] ? Number(m[2]) : st.size - 1, st.size - 1);
    if (start > end) { res.writeHead(416, { "Content-Range": `bytes */${st.size}` }).end(); return; }
    res.writeHead(206, { ...headers, "Content-Range": `bytes ${start}-${end}/${st.size}`, "Content-Length": end - start + 1 });
    if (req.method === "HEAD") return res.end();
    fs.createReadStream(file, { start, end }).pipe(res);
    return;
  }
  res.writeHead(200, { ...headers, "Content-Length": st.size });
  if (req.method === "HEAD") return res.end();
  fs.createReadStream(file).pipe(res);
});
// The client's default socket URL is ws://<this host>/ws/<addr>:<port>: hand that upgrade to the bridge (path minus /ws).
server.on("upgrade", (req, sock, head) => {
  if (!req.url.startsWith("/ws/")) return sock.destroy();
  const up = net.connect(bridgePort, "127.0.0.1", () => {
    const lines = [`${req.method} ${req.url.slice(3)} HTTP/1.1`];
    for (let i = 0; i < req.rawHeaders.length; i += 2) lines.push(`${req.rawHeaders[i]}: ${req.rawHeaders[i + 1]}`);
    up.write(lines.join("\r\n") + "\r\n\r\n");
    if (head.length) up.write(head);
    up.pipe(sock).pipe(up);
  });
  up.on("error", () => sock.destroy());
  sock.on("error", () => up.destroy());
});
server.listen(port, "127.0.0.1", () => console.log(`http://127.0.0.1:${port}/  data: ${gameDir}  ws -> bridge :${bridgePort}`));
