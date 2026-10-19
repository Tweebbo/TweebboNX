// Creates disposable characters on the loopback fixture. Codes stay in memory.
import assert from 'node:assert/strict';
const base='http://127.0.0.1:18997';
const sockets=[];
const variable=n=>{const b=[];do{const v=n&127;n=Math.floor(n/128);b.push(v|(n?128:0));}while(n);return b;};
const string=s=>{const b=new TextEncoder().encode(s);return [...variable(b.length),...b];};
async function connect(device,token=''){
  const ws=new WebSocket(base.replace('http:','ws:')+'/ws/recibidor');sockets.push(ws);ws.binaryType='arraybuffer';
  return new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>reject(Error('Native identity welcome timed out')),8000);
    ws.onopen=()=>ws.send(new Uint8Array([1,6,0,...string(token),0,1,0,127,device,7]));
    ws.onerror=()=>{clearTimeout(timer);reject(Error('Local identity fixture connection failed'));};
    ws.onmessage=e=>{
      const b=new Uint8Array(e.data);if(b[0]!==101)return;
      let p=1;const num=()=>{let n=0,k=1,v;do{v=b[p++];n+=(v&127)*k;k*=128;}while(v&128);return n;};
      const str=()=>{const n=num(),s=new TextDecoder().decode(b.subarray(p,p+n));p+=n;return s;};
      num();const name=str(),code=str();num();const guest=num();clearTimeout(timer);resolve({ws,name,code,guest});
    };
  });
}
try{
  const a=await connect(2),b=await connect(3);assert(a.guest!==b.guest);
  const verify=await fetch(base+'/api/identity',{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({token:a.code})});
  assert.equal(verify.status,200);const identity=await verify.json();assert.equal(identity.id,a.guest);assert.equal(identity.name,a.name);
  assert.equal(verify.headers.get('cache-control'),'no-store');
  await Promise.all([a,b].map(({ws})=>new Promise(resolve=>{ws.addEventListener('close',resolve,{once:true});ws.close();})));
  await new Promise(resolve=>setTimeout(resolve,200));
  const restored=await connect(3,a.code);assert.equal(restored.guest,a.guest);assert.equal(restored.name,a.name);
  const rejected=await fetch(base+'/api/identity',{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({token:'invalid-disposable-character-code'})});
  assert.equal(rejected.status,401);
  const badges=await fetch(base+`/api/badges?guest=${restored.guest}`);assert.equal(badges.status,200);assert(Array.isArray((await badges.json()).badges));
  console.log('OK: private identity verification, Switch character restored on Vita, invalid-code rejection and badge lookup.');
}finally{for(const ws of sockets)ws.close();}
