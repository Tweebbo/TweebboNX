# Switch showcase review — 2026-10-18 (1.0.8)

The port is suitable for a controlled LAN/offline technical demonstration after the fixes
below, subject to a smoke test on the actual Switch. PC rendering does not validate libnx,
the Switch GPU driver, Joy-Con/touch input, dock transitions or microSD latency.

## Corrections

- Furniture cache: the victim-selection condition reused an occupied slot while empty slots
  remained. Alternating furniture could repeatedly decode and upload PNGs during drawing.
  Empty slots now take priority, occupied slots use LRU, and RGBA texture accounting is capped
  at 64 MiB for the supplied assets. The old 512-entry limit could hold about 306 MiB with the
  largest supplied furniture textures. Avatar/font atlases use another 21 MiB of RGBA pixels;
  these figures exclude driver allocations, backgrounds, surfaces and application memory.
- Logging: parenthesize the ring-size macro so modulo arithmetic advances correctly; clamp
  `vsnprintf`'s would-have-written length before indexing the stack buffer. Drop whole lines
  when full, check worker creation, and drain/join the worker during shutdown.
- Networking: use nonblocking TCP connect with a deadline and cancellation checks. Exiting or
  switching offline can interrupt connection setup instead of joining an indefinitely blocked
  connect. Handle allocation failure and zero-byte writes. Check network worker creation and
  synchronize reads of the shutdown flag.
- Storage: write settings and character data on a worker, using a temporary file and rename.
  Existing destinations use a backup/restore fallback for Switch fsdev rename semantics
  ([libnx source](https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/devices/fs_dev.c)).
  Reads see pending writes, including an immediate offline/hotel switch. Shutdown drains the
  bounded queue. This removes file writes from the drawing thread; startup/settings reads and
  initial texture loads still perform filesystem reads.
- Input: initialize the touchscreen, handle the first contact once, and use one touch source.
  Correct the stick's vertical direction to match the D-pad.
- Startup: check RomFS, required atlases and font, clean up a partially initialized renderer,
  release cached furniture on shutdown, and reject scene page counts exceeding the texture array.
- Offline return: preserve a hotel selected through `server.txt` when switching back online,
  and keep the saved hotel address when entering offline mode.

- Device badges: retain announcements that arrive before the room snapshot, preserve them
  across snapshot refreshes, and discard removed entities. The Switch no longer falls back
  to a 3DS badge because the entity did not exist when DEVICE arrived. Resumed sessions
  restore the local device from the build when the server omits an unchanged announcement. Offline uses the local
  build's device type. Switching modes also releases the old room-rights JSON.

The shared `net.c`, `show.c` and `world.c`/`world.h` fixes are mirrored in the Vita source as required by AGENTS.md.
This review does not certify or rebuild the Vita release.

## Verification

- Clean devkitPro/libnx build produces `out/tweebbo.nro`.
- `tests/run.sh`: AddressSanitizer and UndefinedBehaviorSanitizer regression checks for TCP
  timeout/cancellation, furniture cache reuse/eviction/memory budget, oversized log messages,
  ring drainage, asynchronous settings visibility and persistence (including a filesystem
  that refuses to rename over an existing file), and device announcements before ROOM.
- `TWEEBBO_URL=ws://127.0.0.1:8096 node ../Tweebbo-Vercel/tests/device-announcement.mjs`:
  device announcements and Switch/Vita chat reach the local room.
- Desktop harness renders `recibidor` with 53 furniture items and enters offline mode when the
  hotel is unavailable. This compiles the shared client and Switch renderer/transport, with
  desktop substitutes for the platform main and storage.

- The instrumented desktop client completed 255 seconds, with button actions, offline/hotel
  switching and the 240-second connection renewal; it ended online and in the room with no
  ASan/UBSan error. A later short run checks the final device-ordering and saved-address fixes.
  The soak wrapper was edited while its child was running and reported a trailing shell
  error after the client exited; the final, unchanged wrapper subsequently passed.
  LeakSanitizer is disabled for the SDL/Mesa integration run; the isolated regression suite
  uses the sanitizer defaults.
- A TCP fixture that accepts but never completes the WebSocket handshake still allows the
  offline action at 200 ms and a normal exit at 1.5 seconds.
- The final harness run uses an unreachable compiled default and a working saved hotel,
  asserting that switching offline/back retains the saved address and reports device 2.

The build retains existing compiler warnings about bounded string truncation in shared code;
these are not a claim of a warning-free build.

## Console demonstration

Use Homebrew Menu through title override (hold R while launching a game), as recommended in
the install guide. The Album/applet memory limit has not been validated. Use the PC's LAN
hotel on port 8096 and keep a working offline fallback available with Minus.

Before presenting, run on the actual console for at least five minutes: join the room, walk,
wave/chat/change the model, zoom, tap, switch offline and back, and exit with Plus. Check the
same character from the browser. If using a dock or capture card, include that exact setup.

This remains a showcase: catalogue, inventory, friends, building and the other full-client
features listed in README are outside this demo. Desktop frame rates are not Switch FPS.

The current hotel does not resend its full peer-device list on every resumed session. If a
remote player has not been announced to this client, its native badge can use the legacy
3DS fallback until a new announcement arrives. The local Switch badge is restored correctly;
this does not affect movement or chat. Server-side resynchronization is outside these client changes.
