# Web player

The web player is the engine built to WebAssembly with Emscripten, plus a small
JavaScript wrapper. The chili creator's Play panel and the Chili Platform's
`/play` page use it.

| File | What |
|------|------|
| `chili-beat.js`, `chili-beat.wasm` | The engine. `chili-beat.js` is an ES module that exports `createChiliBeat()`. |
| `player.js` | `ChiliBeatPlayer`: draws to a canvas, reads keys, plays sound. |
| `index.html` | A demo page that plays Bee Home and any `.beat` file you pick. |

Each [release](https://github.com/chili-chip/chili-beat/releases) has the first
three files attached. CI also keeps them as the `chili-beat-web` build artifact.

## Play a game on a page

```html
<canvas id="screen" style="width: 512px; height: 512px"></canvas>
<script type="module">
  import createChiliBeat from "./chili-beat.js";
  import { ChiliBeatPlayer } from "./player.js";

  const player = await ChiliBeatPlayer.create(createChiliBeat);
  const error = player.load(await (await fetch("my-game.beat")).text());
  if (error) alert(error);
  else player.start(document.getElementById("screen"));
</script>
```

| Key | Button |
|-----|--------|
| Arrows, WASD | D-pad |
| Z, Space, Enter | A |
| X | B |

Sound starts after the first key press, because browsers block sound until then.

## `ChiliBeatPlayer`

| Member | Does |
|--------|------|
| `ChiliBeatPlayer.create(createChiliBeat, options?)` | Loads the wasm. `options` go to Emscripten, for example `locateFile`. |
| `load(text)` | Loads `.beat` text. Returns `null`, or an error such as `rules[1].do[0].become: no thing has key "z"`. |
| `start(canvas, { keys })` | Runs the game at 60 steps a second in the canvas. Keys are read from `keys` (default `window`). |
| `stop()` | Stops running and silences the sound. |
| `restart()` | Starts the game again. |
| `press(button, down)` | Press or release a button from your own controls, for example touch buttons. Use `BUTTONS.up` … `BUTTONS.b`. |
| `muted` | Set to `true` to silence the sound. |
| `enableSound()` | Turns sound on from your own click handler, for example a "Play tune" button. Keys turn it on by themselves. |
| `onStep` | If set, called after every step with `debug()`. |
| `step(buttons)` | Runs one step yourself (tests, replays). |
| `debug()` | `{ state, screen, beat, counters, fired, sounds, text }`. |
| `rgba()` | The screen as 128 × 128 RGBA bytes. |
| `sound()` | The 4 sound channels: `{ wave, volume, frequency }`. |

### Rule trace

The creator lights up the rule cards that just ran. Collect `fired` from every step:

```js
const lit = new Set();
player.onStep = (debug) => debug.fired.forEach((rule) => lit.add(rule));
// Each animation frame: light the cards in `lit`, then lit.clear().
```

`fired` holds rule indexes, in the order of the file's `rules` list.

## C API

`player.js` wraps these. Other hosts can call them directly.

| Function | Does |
|----------|------|
| `int cb_load(const char* text)` | 1 on success, 0 on error. |
| `const char* cb_error()` | The last load error. |
| `void cb_step(int buttons)` | One step. Up 1, down 2, left 4, right 8, A 16, B 32. |
| `void cb_restart()` | Start again. |
| `const uint8_t* cb_rgba()` | 128 × 128 × 4 bytes of RGBA. |
| `const uint8_t* cb_sound()` | 4 × 4 bytes: wave, volume, frequency low, frequency high. |
| `const char* cb_debug()` | State as JSON, see `debug()` above. |

## Build it

You need [Emscripten](https://emscripten.org/docs/getting_started/downloads.html).

```bash
emcmake cmake -S hosts/wasm -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
node hosts/wasm/test.mjs build-wasm     # plays Bee Home to a win
python -m http.server -d build-wasm     # then open http://localhost:8000
```
