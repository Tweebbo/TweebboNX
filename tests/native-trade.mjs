// Disposable loopback hotel only; never prints character tokens.
import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
const base='ws://127.0.0.1:18997';
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const variable=n=>{const b=[];do{const v=n&127;n=Math.floor(n/128);b.push(v|(n?128:0));}while(n);return b;};
const string=s=>{const b=new TextEncoder().encode(s);return [...variable(b.length),...b];};
class Reader {
  constructor(bytes){this.b=bytes;this.p=1;}
  byte(){assert(this.p<this.b.length);return this.b[this.p++];}
  number(){let n=0,m=1,b;do{b=this.byte();n+=(b&127)*m;m*=128;}while(b&128);return n;}
  short(){return this.byte()|this.byte()<<8;}
  string(){const n=this.number();assert(this.p+n<=this.b.length);const s=new TextDecoder().decode(this.b.subarray(this.p,this.p+n));this.p+=n;return s;}
}
async function wait(check,label){for(let i=0;i<200;i++){if(check())return;await pause(40);}throw Error(`Timed out: ${label}`);}
const clients=[];
class Peer {
  constructor(device,room='recibidor',token=''){
    this.inventory=new Map();this.seq=0;this.ws=new WebSocket(`${base}/ws/${room}`);clients.push(this);this.ws.binaryType='arraybuffer';
    this.ws.onopen=()=>this.ws.send(new Uint8Array([1,6,0,...string(token),0,1,0,70,device,1]));
    this.ws.onmessage=e=>{
      const bytes=new Uint8Array(e.data),r=new Reader(bytes);
      if(bytes[0]===101){this.id=r.number();r.string();this.token=r.string();r.number();this.guest=r.number();}
      if(bytes[0]===108){this.inventory.clear();for(let n=r.number();n;n--)this.inventory.set(r.number(),r.short());this.loaded=true;}
      if(bytes[0]===114)this.inventory.set(r.number(),r.short());
      if(bytes[0]===115)this.inventory.delete(r.number());
      if(bytes[0]===139){this.trade=JSON.parse(r.string());this.seq++;this.onTrade?.(this.trade);}
    };
  }
  ready(){return wait(()=>this.token&&this.loaded,'native welcome and inventory');}
  send(action,extra={}){this.ws.send(new Uint8Array([21,...string(JSON.stringify({action,revision:this.trade?.revision??0,...extra}))]));}
  async act(action,extra={}){const seq=this.seq;this.send(action,extra);await wait(()=>this.seq>seq,action);return this.trade;}
  async buy(){const before=new Set(this.inventory.keys());this.ws.send(new Uint8Array([6,1,0]));await wait(()=>[...this.inventory.keys()].some(id=>!before.has(id)),'purchase');return [...this.inventory.keys()].find(id=>!before.has(id));}
  async room(){const r=await fetch(base.replace('ws:','http:')+'/api/rooms',{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({token:this.token,title:'Native trading check',model:'model_a'})});assert.equal(r.status,200);return (await r.json()).slug;}
}
try {
  const initial=new Peer(3);await initial.ready();const room=await initial.room();initial.ws.close();await pause(150);
  const b=new Peer(3,room,initial.token);await b.ready();const bi=await b.buy();
  if(process.argv.includes('--partner')){
    const sent=new Set();
    b.onTrade=s=>{
      if(['done','cancelled','error'].includes(s.stage))sent.clear();
      const action=s.stage==='invite'?'accept':s.stage==='open'&&!s.mine.length?'add':s.stage==='open'&&s.mine.length&&s.theirs.length&&!s.okMine?s.confirming?'confirm':'ready':null;
      const key=`${s.revision}:${action}`;
      if(action&&!sent.has(key)){sent.add(key);b.send(action,action==='add'?{id:b.inventory.keys().next().value}:{});}
      if(s.stage==='done')console.log('OK: native partner completed furniture exchange.');
    };
    writeFileSync('/tmp/tweebbo-trade-room',room);console.log('Native trade partner ready.');
    await new Promise(resolve=>{const timer=setTimeout(resolve,180000);process.once('SIGTERM',()=>{clearTimeout(timer);resolve();});});
  } else {
    const a=new Peer(2,room);await a.ready();const ai=await a.buy(),extra=await a.buy();
    await a.act('invite',{target:b.id});await wait(()=>b.trade?.stage==='invite','invitation');
    await b.act('accept');await wait(()=>a.trade?.stage==='open','acceptance');
    await a.act('add',{id:ai});await b.act('add',{id:bi});await wait(()=>a.trade.theirs.length===1,'peer offer');
    await a.act('ready');await b.act('ready');await wait(()=>a.trade.confirming,'final review');
    const stale=a.trade.revision;await a.act('confirm');
    assert(a.inventory.has(ai)&&b.inventory.has(bi),'A single confirmation must not move furniture');
    await a.act('add',{id:extra});await wait(()=>b.trade.revision===a.trade.revision,'updated revision');
    await b.act('confirm',{revision:stale});assert(!b.trade.okMine&&!b.trade.confirming);
    await a.act('remove',{id:extra});await wait(()=>b.trade.mine.length===1&&b.trade.theirs.length===1,'removed item');
    await a.act('ready');await b.act('ready');await wait(()=>a.trade.confirming,'second final review');
    await a.act('confirm');await b.act('confirm');await wait(()=>a.trade.stage==='done'&&b.trade.stage==='done','completed swap');
    assert(a.inventory.has(bi)&&!a.inventory.has(ai)&&a.inventory.has(extra));assert(b.inventory.has(ai)&&!b.inventory.has(bi));
    await a.act('invite',{target:b.id});await b.act('decline');await wait(()=>a.trade.stage==='cancelled','decline');
    await a.act('invite',{target:b.id});await b.act('accept');await a.act('cancel');await wait(()=>b.trade.stage==='cancelled','cancellation');
    console.log('OK: Switch/Vita trade invitation, offers, two confirmations, stale revision rejection, exact inventory swap, decline and cancellation.');
  }
} finally {for(const c of clients)c.ws.close();}
