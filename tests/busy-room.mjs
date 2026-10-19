// Run only against a disposable local hotel: SERVER=ws://127.0.0.1:18997 node tests/busy-room.mjs.
import assert from 'node:assert/strict';
const base = process.env.SERVER || 'ws://127.0.0.1:18997';
assert.equal(new URL(base).hostname, '127.0.0.1');
assert.equal(new URL(base).protocol, 'ws:');
const clients = [];
const ids = new Set();
let chats = 0;
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
try {
  for (let i = 0; i < 32; i++) {
    await new Promise((resolve, reject) => {
      const ws = new WebSocket(`${base}/ws/recibidor`);
      clients.push(ws);
      ws.binaryType = 'arraybuffer';
      const timeout = setTimeout(() => reject(new Error('Guest did not join')), 10000);
      ws.onopen = () => ws.send(new Uint8Array([1, 6, 0, 0, 0, 1, 0, 66, i % 3 + 1]));
      ws.onerror = () => reject(new Error('Guest connection failed'));
      ws.onmessage = ({data}) => {
        const bytes = new Uint8Array(data);
        if (bytes[0] === 101) {
          let id = 0, shift = 0, at = 1, b;
          do { b = bytes[at++]; id += (b & 127) * 2 ** shift; shift += 7; } while (b & 128);
          ids.add(id); clearTimeout(timeout); resolve();
        }
        if (bytes[0] === 106) chats++;
      };
    });
  }
  assert.equal(ids.size, 32);
  console.log('32 independent native guests joined the disposable lobby.');
  const started = Date.now();
  let turn = 0;
  while (Date.now() - started < Number(process.env.DURATION_MS || 285000)) {
    const ws = clients[turn % clients.length];
    assert.equal(ws.readyState, WebSocket.OPEN);
    const message = new TextEncoder().encode(`Load message ${turn}: canción, pingüino.`);
    ws.send(new Uint8Array([3, message.length, ...message]));
    ws.send(new Uint8Array([2, 3 + turn % 6, 4 + turn % 8]));
    turn++;
    await sleep(500);
  }
  assert(chats > 1000, `Expected sustained chat delivery, received ${chats}`);
  console.log(`OK: 32 connected guests, ${turn} chat/walk requests and ${chats} chat deliveries.`);
} finally {
  for (const ws of clients) ws.close();
}
