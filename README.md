# Tweebbo for Nintendo Switch

Release 1.3.5, 2026-10-23: room layouts P, Q, R, S, X and Z use regenerated Kepler-style doorways (room images and maps repacked from the 3DS export). Builds verified; physical hardware still pending.

Published follow-up, 2026-10-23: version 1.3.4 adds doorbell admission, profile badge selection and the Tweebbo Club catalogue shortcut, and fixes furniture animation and bear/terrier/pig rendering. The update is optional; existing clients remain admitted. Release builds and uploaded artifact hashes were verified. Physical-device checks remain pending. Older local-only notes below are historical.

Notice follow-up, 2026-10-23: the thick black stripe above purchase notices is removed. The shared content-frame renderer now uses one row of the original top-border sprite and fills its remaining interior with the panel background.

Local follow-up, 2026-10-23: select Lia in reception and choose Suggestions to submit a suggestion or bug report. Seated wave, kiss, laugh, thumbs-up and sign preserve the seat. Window corner joins and rounded fills are corrected. The server records Switch separately. NRO rebuilt; desktop harness verified, physical hardware still pending. Review: `../Tweebbo-Vercel/docs/reviews/console-polish-20261023/`.

Local candidate, 2026-10-22: F3 and all four regional lobbies are packed. The navigator uses English PC featured priority. Keep a Camera in inventory, select it and choose Take photo for capture, zoom, caption and save. Saved photos can be placed on walls; opening their stored image still requires the web client. This source work is not yet a published release. Team evidence is in `../Tweebbo-Vercel/docs/reviews/console-polish-20261023/`.

Play Tweebbo on your Nintendo Switch, in the same hotel as [tweebbo.lol](https://tweebbo.lol).

Also on [Nintendo 3DS](https://github.com/Tweebbo/tweebbo-3ds) and [PlayStation Vita](https://github.com/Tweebbo/Tweebbo-Vita).

![Download QR](download-qr.png)

## Download

**[Download Tweebbo for Nintendo Switch](https://github.com/Tweebbo/TweebboNX/releases/latest)**

Download `tweebbo.nro` from the latest release.

## How to install

1. You need a Nintendo Switch with homebrew enabled (Atmosphere and the Homebrew Menu).
2. Download `tweebbo.nro`.
3. Copy it to your SD card, for example to `switch/Tweebbo/tweebbo.nro`.
4. Open the Homebrew Menu (hold **R** while starting a game from the HOME Menu) and choose **Tweebbo**.
5. Connect to the internet and play. If there is no connection, choose **Play offline** on the start screen.

## Controls

The room shows a small button legend in the top-left corner.

- **A**: talk or use furniture
- **Y**: chat · **X**: wave · **B**: rooms or close
- **L/R**: room zoom · **+**: quit

In the Navigator and the large furniture windows:

- **D-pad**: move · **A**: choose or place · **B**: close or cancel
- **L/R**: turn pages, or rotate and change height while placing
- **ZL/ZR**: in the room, open Tools; change catalogue section; in the Navigator, also turn pages

Unpublished local update: new direction presses and release/repress respond immediately,
while held input still repeats at a bounded rate. In Chat, L shows older messages and
R shows newer messages; incoming messages preserve the reading position. The matching
local server supports the Igloo and its animated visitors. See
[validation and limitations](../Tweebbo-Vercel/docs/performance/console-input/README.md).

**Support** is in the bottom toolbar and in Tools. The $5 donation gives Tweebbo Club
for life, the VIP badge and 10,000 credits the first time. In a changing booth,
**D-pad** moves between the arrows and buttons, **L/R** changes the swimsuit colour,
**A** confirms the focused control and **B** exits in normal clothes.

## Updates

When you enter the hotel, Tweebbo checks GitHub for a newer release and offers **Update now**. The
download resumes if the connection drops and is verified against the release SHA-256 before the
old `tweebbo.nro` is replaced (the previous file is kept as `tweebbo.nro.bak`). Close Tweebbo and
open it again from the Homebrew Menu to use the new version. **Help → Search for updates** checks
again at any time.

## Notes

- Free to play. No Nintendo account is needed.
- Tweebbo is not affiliated with Nintendo.

### Local room names and Classic missions (2026-10-20)

Owners can rename rooms without changing their links or contents; the five-room creation limit is removed on the server. Room titles update live through optional server packet 144. My info in Friends edits the personal mission (up to 50 Unicode characters, including an empty mission). These changes are local only and require the updated local server.

Enter your room and open Hotel → Room settings → Rename room. Friends → My info edits the mission; the selected contact displays their mission.
