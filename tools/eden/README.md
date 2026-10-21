# Eden local validation

Eden v0.2.1 Linux amd64 clang PGO successfully loaded the current local NRO,
connected to isolated hotel 18124 and rendered Igloo. Emulated controller input
opened the keyboard, entered and sent `q`, and opened Navigator. Screenshots and
the exact emulator/NRO hashes are in
`../../../Tweebbo-Vercel/docs/reviews/console-assets-20261020/eden/`.

The official download was selected through https://eden-emu.dev/downloads/:
https://stable.eden-emu.dev/v0.2.1/Eden-Linux-v0.2.1-amd64-clang-pgo.AppImage.
The extracted AppImage and isolated profile are under `tools/eden-cache/`, which
is ignored by Git. No console keys or firmware were downloaded. Dismissing the
initial missing-keys prompt with No permitted this unencrypted homebrew NRO to run.

The initial container lacked Vulkan drivers. The sibling Containerfile adds Mesa
Vulkan to the existing `tweebbo-3ds-emu` image. The successful run used Xvfb :97,
Vulkan llvmpipe 25.2.8 and SHARUN_ALLOW_SYS_VKICD=1. It did not use a physical GPU.
Eden logged invalid shader instructions despite rendering the room and controls;
these messages remain unresolved. Audio was not validated. The observed FPS is
not a Switch hardware benchmark or a high-load test.

To reproduce, build the image from the repository root:

```sh
podman build -t tweebbo-eden -f tools/eden/Containerfile .
```

Run the extracted AppRun with `/work/out/tweebbo.nro` inside this image, mounting
the repository at `/work` and using host networking for the isolated hotel. Use:

```text
DISPLAY=:97
XDG_CONFIG_HOME=/work/tools/eden-cache/config
XDG_DATA_HOME=/work/tools/eden-cache/data
XDG_CACHE_HOME=/work/tools/eden-cache/cache
SHARUN_ALLOW_SYS_VKICD=1
```

Start `Xvfb :97 -screen 0 1280x800x24` before AppRun. The emulated SD profile is
`tools/eden-cache/data/eden/sdmc/switch/Tweebbo/`; its server.txt selects
`ws://127.0.0.1:18124` and room.txt selects `igloo`. Keep account tokens in that
ignored profile, out of logs and reports. Default keyboard mappings used here:
C = A, X = B, V = X, Z = Y. Capture with ImageMagick `import -window root` on
DISPLAY :97. Stop the test container after collecting evidence.

The NRO was not rebuilt or modified for this validation. No GitHub release or
production deployment was performed.
