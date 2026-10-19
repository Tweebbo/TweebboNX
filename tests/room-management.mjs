// Uses new guests and a new room on a disposable loopback hotel. Never use a live hotel.
import assert from 'node:assert/strict';
const base = process.env.SERVER || 'ws://127.0.0.1:18997';
assert.equal(new URL(base).hostname, '127.0.0.1');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
function variable(n) {
  const out = [];
  do { const b = n & 127; n = Math.floor(n / 128); out.push(b | (n ? 128 : 0)); } while (n);
  return out;
}
function string(s) { const b = new TextEncoder().encode(s); return [...variable(b.length), ...b]; }
function reader(bytes) {
  let at = 1;
  const number = () => { let n = 0, shift = 0, b; do { b = bytes[at++]; n += (b & 127) * 2 ** shift; shift += 7; } while (b & 128); return n; };
  return {number, string() { const n = number(); const s = new TextDecoder().decode(bytes.subarray(at, at + n)); at += n; return s; }};
}
const clients = [];
async function connect(slug, device, token = '') {
  const ws = new WebSocket(`${base}/ws/${slug}`), packets = [];
  clients.push(ws); ws.binaryType = 'arraybuffer';
  ws.onopen = () => ws.send(new Uint8Array([1, 6, 0, ...string(token), 0, 1, 0, 66, device]));
  ws.onmessage = e => packets.push(new Uint8Array(e.data));
  ws.onerror = () => {};
  const wait = async (kind, start = 0, predicate = () => true) => {
    const deadline = Date.now() + 8000;
    while (Date.now() < deadline) {
      const found = packets.slice(start).find(p => p[0] === kind && predicate(p));
      if (found) return found;
      await delay(20);
    }
    throw new Error(`Timed out waiting for packet ${kind}`);
  };
  const welcome = reader(await wait(101));
  const id = welcome.number(); welcome.string();
  const identity = welcome.string();
  return {ws, packets, wait, id, token: identity};
}
const rights = p => JSON.parse(reader(p).string());
try {
  const initial = await connect('recibidor', 2);
  const response = await fetch(`${base.replace(/^ws/, 'http')}/api/rooms`, {
    method: 'POST', headers: {'content-type': 'application/json'},
    body: JSON.stringify({title: 'Native access test', model: 'model_a', token: initial.token}),
  });
  assert.equal(response.status, 200);
  const room = await response.json();
  assert.equal(room.token === initial.token, true, 'Room creation must preserve the owner identity');
  initial.ws.close(); await delay(100);
  const owner = await connect(room.slug, 2, initial.token);
  assert.equal(rights(await owner.wait(136)).owner, true);
  const visitor = await connect(room.slug, 3);
  assert.equal(rights(await visitor.wait(136)).rights, false);
  let start = visitor.packets.length;
  owner.ws.send(new Uint8Array([20, 1, ...variable(visitor.id), 0]));
  await visitor.wait(136, start, p => rights(p).rights === true);
  start = visitor.packets.length;
  owner.ws.send(new Uint8Array([20, 2, ...variable(visitor.id), 0]));
  await visitor.wait(136, start, p => rights(p).rights === false);
  start = visitor.packets.length;
  const ownerStart = owner.packets.length;
  owner.ws.send(new Uint8Array([20, 5, ...variable(visitor.id), 1]));
  await visitor.wait(137, start);
  const banned = rights(await owner.wait(136, ownerStart, p => rights(p).bans?.length > 0));
  start = owner.packets.length;
  owner.ws.send(new Uint8Array([20, 6, ...variable(banned.bans[0].guest), 0]));
  await owner.wait(136, start, p => rights(p).bans?.length === 0);
  const lock = async (token, password) => fetch(`${base.replace(/^ws/, 'http')}/api/rooms/lock`, {
    method: 'POST', headers: {'content-type': 'application/json', authorization: `Bearer ${token}`},
    body: JSON.stringify({slug: room.slug, password}),
  });
  assert.equal((await lock(visitor.token, 'test-access')).status, 403);
  assert.equal((await lock(owner.token, 'abc')).status, 400);
  for (const password of ['test-access', 'ñ'.repeat(32), '']) {
    const saved = await lock(owner.token, password);
    assert.equal(saved.status, 200);
    assert.equal((await saved.json()).locked, Boolean(password));
    const listing = await fetch(`${base.replace(/^ws/, 'http')}/api/rooms?scope=mine`, {
      headers: {authorization: `Bearer ${owner.token}`},
    });
    assert.equal((await listing.json()).rooms.find(r => r.slug === room.slug).locked, Boolean(password));
  }
  console.log('OK: owner-only room password setting, replacement, Unicode limits, removal and persisted access status.');
  console.log('OK: native room creation, owner identity, grant/revoke rights, ban and unban across Switch/Vita devices.');
} finally {
  for (const ws of clients) ws.close();
}
