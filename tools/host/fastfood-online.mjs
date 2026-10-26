import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {once} from 'node:events';
const server=process.env.SERVER||'ws://127.0.0.1:18877';
assert(['127.0.0.1','localhost'].includes(new URL(server).hostname),'Use a disposable loopback hotel.');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function wait(test,message,timeout=20000){const end=Date.now()+timeout;while(Date.now()<end){if(test())return;await sleep(50);}throw Error(message);}
function variable(n){const bytes=[];do{bytes.push(n%128|(n>=128?128:0));n=Math.floor(n/128);}while(n);return bytes;}
function string(s){return [...variable(Buffer.byteLength(s)),...Buffer.from(s)];}
const hotel=new WebSocket(server+'/ws/snowstorm');hotel.binaryType='arraybuffer';let token='',game=0,started=false,plate=false,chute=false,host;
hotel.addEventListener('message',e=>{const b=Buffer.from(e.data);if(b[0]!==101)return;let i=1;const v=()=>{let n=0,m=1,c;do{c=b[i++];n+=(c&127)*m;m*=128;}while(c&128);return n;};const s=()=>{const n=v(),t=b.subarray(i,i+n).toString();i+=n;return t;};v();s();token=s();});
let food;
try{
 await once(hotel,'open');hotel.send(new Uint8Array([1,6,0,...string(''),0,0,0,17]));await wait(()=>token,'Hotel did not admit test peer');
 food=new WebSocket(server+'/ws/fastfood');
 food.addEventListener('message',e=>{const m=JSON.parse(e.data);if(m.type==='lobby'){
   if(m.current)game=m.current;
   const g=m.games.find(g=>g.id===game);
   if(g?.players.length===2&&!started){started=true;food.send('{"op":"start"}');}
 }else if(m.type==='state'){
   const p=m.state.players[1];if(p?.plate){plate=true;if(p.plate.big)chute=true;}
 }});
 await once(food,'open');food.send(JSON.stringify({version:2,token}));food.send('{"op":"create","goal":18}');await wait(()=>game,'Peer could not create game');
 host=spawn('tools/host/run.sh',[],{env:{...process.env,SERVER:server,ROOM:'snowstorm',FASTFOOD_GAME:String(game),UI_EXERCISE:'fastfood-online',QUIT_MS:'50000',SHOT_MS:'8000'},stdio:'inherit'});
 const [code]=await once(host,'exit');assert.equal(code,0,'Native client failed');assert(started&&plate&&chute,'Native input did not reach the authoritative peer');
 console.log('PASS: native JSON WebSocket joins browser-protocol peer; drop and big parachute reach server and peer.');
}finally{host?.kill();food?.close();hotel.close();}
