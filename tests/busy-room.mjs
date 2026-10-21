// Run only against a disposable local hotel: SERVER=ws://127.0.0.1:18997 node tests/busy-room.mjs.
import assert from 'node:assert/strict';
const base = process.env.SERVER || 'ws://127.0.0.1:18997';
assert.equal(new URL(base).hostname, '127.0.0.1');
assert.equal(new URL(base).protocol, 'ws:');
const clients = [];
const count = Number(process.env.CLIENTS || 32);
assert(Number.isInteger(count) && count > 0 && count <= 100);
const rooms=(process.env.ROOMS||'recibidor').split(',');
assert(rooms.every(room=>/^[a-z0-9-]+$/.test(room)));
const ids = new Set();
let chats = 0;
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
try {
  for (let i = 0; i < count; i++) {
    await new Promise((resolve, reject) => {
      const ws = new WebSocket(`${base}/ws/${rooms[i%rooms.length]}`);
      clients.push(ws);
      ws.binaryType = 'arraybuffer';
      const timeout = setTimeout(() => reject(new Error(`Guest ${i+1}/${count} did not join`)), 10000);
      ws.onopen = () => ws.send(new Uint8Array([1, 6, 0, 0, 0, 1, 0, 126, i % 3 + 1, 15]));
      ws.onerror = () => reject(new Error('Guest connection failed'));
      ws.onmessage = ({data}) => {
        const bytes = new Uint8Array(data);
        if(bytes[0]===132) { clearTimeout(timeout); reject(new Error(`Capacity refused guest ${i+1}/${count}: ${[...bytes]}`)); }
        if (bytes[0] === 101) {
          let id = 0, shift = 0, at = 1, b;
          do { b = bytes[at++]; id += (b & 127) * 2 ** shift; shift += 7; } while (b & 128);
          ids.add(`${rooms[i%rooms.length]}/${id}`); clearTimeout(timeout); resolve();
        }
        if (bytes[0] === 106) chats++;
      };
    });
  }
  assert.equal(ids.size, count);
  console.log(`${count} independent native guests joined ${rooms.length} disposable rooms.`);
  const started = Date.now();
  let turn = 0;
  while (Date.now() - started < Number(process.env.DURATION_MS || 285000)) {
    const ws = clients[turn % clients.length];
    assert.equal(ws.readyState, WebSocket.OPEN);
    const message = new TextEncoder().encode(`Load message ${turn}: canción, pingüino.`);
    ws.send(new Uint8Array([3, message.length, ...message]));
    ws.send(new Uint8Array([2, 3 + turn % 6, 4 + turn % 8]));
    if(process.env.WALK_BURST==='1')for(const peer of clients)peer.send(new Uint8Array([2,3+turn%6,4+turn%8]));
    turn++;
    await sleep(500);
  }
  assert(chats > 1000, `Expected sustained chat delivery, received ${chats}`);
  console.log(`OK: ${count} connected guests, ${turn} chat/walk rounds and ${chats} chat deliveries; walk burst=${process.env.WALK_BURST==='1'}.`);
} finally {
  for (const ws of clients) ws.close();
}
