// Creates disposable identities only on the isolated loopback hotel.
import assert from 'node:assert/strict';
const base=process.env.SERVER||'ws://127.0.0.1:18997';
assert.equal(base,'ws://127.0.0.1:18997');
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const variable=n=>{const out=[];do{const b=n&127;n=Math.floor(n/128);out.push(b|(n?128:0));}while(n);return out;};
class Reader {
  constructor(bytes){this.bytes=bytes;this.at=1;}
  byte(){assert(this.at<this.bytes.length);return this.bytes[this.at++];}
  number(){let n=0,m=1,b;do{b=this.byte();n+=(b&127)*m;m*=128;}while(b&128);return n;}
  string(){const n=this.number();assert(this.at+n<=this.bytes.length);const s=new TextDecoder().decode(this.bytes.subarray(this.at,this.at+n));this.at+=n;return s;}
}
async function wait(check,label){for(let i=0;i<160;i++){const value=check();if(value)return value;await sleep(50);}throw new Error(`Timed out: ${label}`);}
class Peer {
  constructor(device){
    this.packets=[];this.ws=new WebSocket(`${base}/ws/recibidor`);this.ws.binaryType='arraybuffer';
    this.ws.onopen=()=>this.ws.send(new Uint8Array([1,6,0,0,0,1,0,70,device]));
    this.ws.onmessage=e=>this.packets.push(new Uint8Array(e.data));
  }
  async ready(){const r=new Reader(await this.packet(101));this.id=r.number();r.string();this.token=r.string();r.number();this.guest=r.number();await this.packet(102);}
  packet(kind,start=0,check=()=>true){return wait(()=>this.packets.slice(start).find(p=>p[0]===kind && check(p)),`packet ${kind}`);}
  game(op,a,b=0,c=0,d=0){this.ws.send(new Uint8Array([16,op,...variable(a),b,c,d]));}
  async api(body,path='/api/social'){
    const response=await fetch(base.replace('ws:','http:')+path,{method:body?'POST':'GET',headers:{authorization:`Bearer ${this.token}`,'content-type':'application/json'},body:body?JSON.stringify(body):undefined});
    assert.equal(response.status,200);return response.json();
  }
}
function state(packet){
  const r=new Reader(packet);assert.equal(r.byte(),2);
  const g={id:r.number(),kind:r.byte(),players:[r.number(),r.number()],turn:r.byte(),result:r.byte(),reason:r.byte()};
  r.byte();r.byte();r.byte();g.board=Array.from({length:r.number()},()=>r.byte());g.moves=Array.from({length:r.number()},()=>[r.byte(),r.byte()]);return g;
}
const a=new Peer(2),b=new Peer(3);
try {
  await Promise.all([a.ready(),b.ready()]);
  for(const kind of [1,2,3]){
    const start=b.packets.length;a.game(1,b.id,kind);
    await b.packet(127,start,p=>p[1]===1);
    const sa=a.packets.length,sb=b.packets.length;b.game(2,a.id,1);
    const ga=state(await a.packet(127,sa,p=>p[1]===2)),gb=state(await b.packet(127,sb,p=>p[1]===2));assert.deepEqual(ga,gb);assert.equal(ga.kind,kind);
    const current=ga.players[ga.turn]===a.id?a:b;const from=ga.moves[0][0],to=ga.moves[0][1];
    const ma=a.packets.length,mb=b.packets.length;current.game(3,ga.id,from,to,5);
    const moved=state(await a.packet(127,ma,p=>p[1]===2));assert.deepEqual(moved,state(await b.packet(127,mb,p=>p[1]===2)));assert.notDeepEqual(moved.board,ga.board);
    const end=a.packets.length;a.game(4,ga.id);const final=state(await a.packet(127,end,p=>p[1]===2));assert(final.result && final.reason===6);
    await sleep(550);
  }
  await a.api({action:'request',peer:b.guest});
  assert((await b.api()).incoming.some(p=>p.id===a.guest));
  await b.api({action:'accept',peer:a.guest});
  assert((await a.api()).friends.some(p=>p.id===b.guest));
  const message='Private message: ñ, accents áé, and "quotes".';
  await a.api({action:'message',peer:b.guest,text:message});
  const messages=await b.api(null,`/api/social?peer=${a.guest}`);assert(messages.some(m=>m.text===message));
  await b.api({action:'read',peer:a.guest,through:messages.at(-1).id});
  await b.api({action:'block',peer:a.guest});assert((await b.api()).blocked.some(p=>p.id===a.guest));
  await b.api({action:'unblock',peer:a.guest});assert(!(await b.api()).blocked.some(p=>p.id===a.guest));
  console.log('OK: Switch/Vita board-game invitations, legal moves, matching boards, resignation; friend request/accept, private messages, read, block/unblock.');
} finally {a.ws.close();b.ws.close();}
