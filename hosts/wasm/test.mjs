// Runs the wasm build in Node: plays Bee Home to a win.
//   node hosts/wasm/test.mjs build-wasm
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import { pathToFileURL } from "node:url";
import assert from "node:assert/strict";

const dir = resolve(process.argv[2] || "build-wasm");
const { default: createChiliBeat } = await import(pathToFileURL(`${dir}/chili-beat.js`));
const { ChiliBeatPlayer, BUTTONS } = await import(pathToFileURL(`${dir}/player.js`));

const player = await ChiliBeatPlayer.create(createChiliBeat);

assert.match(player.load("{}"), /missing "format"|unknown|format/);
assert.equal(player.load(readFileSync(new URL("../../tests/data/bee-home.beat", import.meta.url), "utf8")), null);

function press(name) {
  player.step(BUTTONS[name]);
  player.step(0);
}
function waitBeats(n) {
  for (let i = 0; i < n; i++) {
    const beat = player.debug().beat;
    while (player.debug().beat === beat) player.step(0);
  }
}

const fired = new Set();
player.onStep = (debug) => debug.fired.forEach((rule) => fired.add(rule));

for (let i = 0; i < 4; i++) press("up");
press("left");
assert.deepEqual(player.debug().counters, [1]);
assert.deepEqual(player.debug().sounds, []);
for (let i = 0; i < 4; i++) {
  waitBeats(8);
  press("left");
}
press("down");
press("down");
press("right");
const end = player.debug();
assert.equal(end.state, "won");
assert.equal(end.text, "The hive is happy.");
assert.ok(fired.has(1) && fired.has(5), "bump and win rules ran");

const pixels = player.rgba();
assert.equal(pixels.length, 128 * 128 * 4);
assert.deepEqual([...pixels.slice(0, 4)], [0x1a, 0x1c, 0x2c, 255]);  // counter strip, top left
assert.deepEqual([...pixels.slice(127 * 4, 128 * 4)], [0x38, 0xb7, 0x64, 255]);  // hedge, top right
assert.equal(player.sound().length, 4);

press("a");
assert.equal(player.debug().state, "playing");
console.log("wasm: ok");
