// Dev server for the browser client: / -> Web/out, /data/ -> the game folder (with HTTP Range, the client reads
// .pak files in pieces). Local only.
// usage: node serve.mjs [port=8080] [gameDir=G:/DEV/hb/run/client] [bridgePort=8000] [botsPort=8097]
// /bots is the page of the server's bots (bots.html), /bots/api/* is proxied to hg on botsPort.
// /stats is the health page of the server (stats.html): /stats/api/stats is hg's snapshot, /stats/api/bridge asks the bridge.
import http from "node:http";
import net from "node:net";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const port = Number(process.argv[2] || 8080);
const gameDir = path.resolve(process.argv[3] || "G:/DEV/hb/run/client");
const bridgePort = Number(process.argv[4] || 8000);
const bridgeHost = process.env.HB_BRIDGE_HOST || "127.0.0.1"; // in containers the bridge is another service
const listenHost = process.env.HB_HOST || "127.0.0.1";
const botsPort = Number(process.argv[5] || 8097); // hg's bots JSON (web-port in Bots.cfg)
const toolsDir = path.dirname(fileURLToPath(import.meta.url));
const outDir = path.resolve(toolsDir, "../out");
const types = { ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".wasm": "application/wasm" };

// Resolves a URL path inside root; null if it would escape it.
function inside(root, urlPath) {
  let p = path.resolve(root, "." + decodeURIComponent(urlPath));
  if (!(p === root || p.startsWith(root + path.sep))) return null;
  // the game data comes from Windows (the client asks for "sprites/trees1.pak", the file is "SPRITES/Trees1.pak"): match names ignoring case
  if (!fs.existsSync(p)) {
    let cur = root;
    for (const seg of path.relative(root, p).split(path.sep)) {
      let names; try { names = fs.readdirSync(cur); } catch { return p; }
      const hit = names.includes(seg) ? seg : names.find((n) => n.toLowerCase() === seg.toLowerCase());
      if (!hit) return p;
      cur = path.join(cur, hit);
    }
    p = cur;
  }
  return p;
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

// a dev page must not take the game page down with it: log what escapes and keep serving
process.on("uncaughtException", (e) => console.error("uncaught:", e));
const server = http.createServer((req, res) => {
  const url = new URL(req.url, "http://x");
  if (url.pathname === "/events") {
    res.writeHead(200, { "Content-Type": "text/event-stream", "Cache-Control": "no-cache" });
    res.write(": hot reload\n\n");
    listeners.add(res);
    req.on("close", () => listeners.delete(res));
    return;
  }
  // the bots page, and the JSON of the game server (hg, web-port in Bots.cfg) behind /bots/api/
  if (url.pathname === "/bots" || url.pathname === "/bots/") {
    res.writeHead(200, { "Content-Type": types[".html"], "Cache-Control": "no-cache" });
    return fs.createReadStream(path.join(toolsDir, "bots.html")).pipe(res);
  }
  if (url.pathname.startsWith("/bots/api/")) {
    // GET and the page's POST /bots/api/max (the X-Bots header goes along: hg refuses a POST without it)
    const up = http.request({ host: "127.0.0.1", port: botsPort, path: url.pathname.slice(5) + url.search, method: req.method === "POST" ? "POST" : "GET", timeout: 3000,
      headers: { "x-bots": req.headers["x-bots"] || "", "content-type": req.headers["content-type"] || "application/json", "content-length": req.method === "POST" ? req.headers["content-length"] || 0 : 0 } }, (r) => {
      res.writeHead(r.statusCode || 502, { "Content-Type": r.headers["content-type"] || "application/json", "Cache-Control": "no-cache" });
      r.pipe(res);
    });
    up.on("error", () => { if (res.headersSent) return res.destroy(); res.writeHead(502, { "Content-Type": "application/json" }).end('{"error":"hg is not serving the bots (web-port)"}'); });
    up.on("timeout", () => up.destroy());
    if (req.method === "POST") req.pipe(up); else up.end();
    return;
  }
  // the health page: the snapshot of hg (stats.rs). The 1.5 s timeout is on purpose: a stalled game loop must show as "hg did not
  // answer" and not as a hung page. The login is not probed (a connection to its client port would count as a player): hg's own
  // snapshot says whether it is registered with it.
  if (url.pathname === "/stats" || url.pathname === "/stats/") {
    res.writeHead(200, { "Content-Type": types[".html"], "Cache-Control": "no-cache" });
    return fs.createReadStream(path.join(toolsDir, "stats.html")).pipe(res);
  }
  if (url.pathname === "/stats/api/stats") {
    const up = http.get({ host: "127.0.0.1", port: botsPort, path: "/api/stats", timeout: 1500 }, (r) => {
      res.writeHead(r.statusCode || 502, { "Content-Type": "application/json", "Cache-Control": "no-cache" });
      r.pipe(res);
    });
    // the connection can also break after the answer started: then only the socket is dropped (writeHead would throw)
    up.on("error", () => { if (res.headersSent) return res.destroy(); res.writeHead(502, { "Content-Type": "application/json" }).end('{"error":"hg is not answering (not running, or web-port off in Bots.cfg)"}'); });
    up.on("timeout", () => { up.destroy(); if (!res.headersSent) res.writeHead(504, { "Content-Type": "application/json" }).end('{"error":"hg did not answer in time"}'); });
    return;
  }
  if (url.pathname === "/stats/api/bridge") {
    const t0 = Date.now();
    const done = (ok) => { if (!res.headersSent) res.writeHead(200, { "Content-Type": "application/json", "Cache-Control": "no-cache" }).end(JSON.stringify({ up: ok, ms: Date.now() - t0 })); };
    // only a TCP connect (the bridge speaks WebSocket and answers a plain GET with a reset)
    const up = net.connect({ host: bridgeHost, port: bridgePort, timeout: 800 }, () => { up.destroy(); done(true); });
    up.on("error", () => done(false));
    up.on("timeout", () => { up.destroy(); done(false); });
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
  const up = net.connect(bridgePort, bridgeHost, () => {
    const lines = [`${req.method} ${req.url.slice(3)} HTTP/1.1`];
    for (let i = 0; i < req.rawHeaders.length; i += 2) lines.push(`${req.rawHeaders[i]}: ${req.rawHeaders[i + 1]}`);
    up.write(lines.join("\r\n") + "\r\n\r\n");
    if (head.length) up.write(head);
    up.pipe(sock).pipe(up);
  });
  up.on("error", () => sock.destroy());
  sock.on("error", () => up.destroy());
});
server.listen(port, listenHost, () => console.log(`http://127.0.0.1:${port}/  data: ${gameDir}  ws -> bridge :${bridgePort}`));
