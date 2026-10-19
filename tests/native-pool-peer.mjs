const ws=new WebSocket('ws://127.0.0.1:18997/ws/rooftop');ws.binaryType='arraybuffer';let me=0,stage=0;
const command=op=>{const b=new TextEncoder().encode(JSON.stringify(op)),head=[22];let n=b.length;while(n>=128){head.push((n&127)|128);n>>>=7;}head.push(n);ws.send(new Uint8Array([...head,...b]));};
ws.onopen=()=>ws.send(new Uint8Array([1,6,0,0,0,1,0,127,1,7]));
ws.onmessage=e=>{
 const bytes=new Uint8Array(e.data);let at=1;const num=()=>{let n=0,k=1,b;do{b=bytes[at++];n+=(b&127)*k;k*=128;}while(b&128);return n;};const str=()=>{const n=num(),s=new TextDecoder().decode(bytes.subarray(at,at+n));at+=n;return s;};
 if(bytes[0]===101)me=num();
 if(bytes[0]!==141)return;const j=JSON.parse(str()),s=j.state;if(!s)return;
 const swimmer=s.swimmers?.[me];
 if(stage===0&&me){stage=1;command({op:'booth'});}
 else if(stage===1&&swimmer?.booth){stage=2;command({op:'outfit',color:4});}
 else if(stage===2&&swimmer?.color===4){stage=3;command({op:'leave'});}
 else if(stage===3&&!swimmer?.booth){stage=4;command({op:'wobble_join'});}
 else if(stage===4&&s.wobble?.phase===2){stage=5;setTimeout(()=>{stage=4;command({op:'wobble_join'});},2500);}
};
ws.onerror=()=>{console.error('Local rooftop peer connection failed');process.exitCode=1;};
process.on('SIGTERM',()=>{ws.close();process.exit(0);});setTimeout(()=>{ws.close();process.exit(0);},900000);
