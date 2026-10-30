# Palette

Every game picks **4 colours** from **Sweetie 16**, a 16-colour palette by
[GrafxKid](https://grafxkid.tumblr.com/). It is one of the most used palettes on
[Lospec](https://lospec.com/palette-list/sweetie-16) and the default palette of
the TIC-80 fantasy console. It has warm and cool ramps, a near-white and a
near-black, so almost any 4 colours from it look good together.

A game stores its colours as 4 numbers, for example `"palette": [0, 6, 4, 12]`.
The first colour is the background. In pictures, digit `0` is that background
(see-through for things), and `1` to `3` are the other three.

<div class="swatches" markdown>
<div class="swatch"><span style="background:#1a1c2c"></span><code>0 #1a1c2c</code></div>
<div class="swatch"><span style="background:#5d275d"></span><code>1 #5d275d</code></div>
<div class="swatch"><span style="background:#b13e53"></span><code>2 #b13e53</code></div>
<div class="swatch"><span style="background:#ef7d57"></span><code>3 #ef7d57</code></div>
<div class="swatch"><span style="background:#ffcd75"></span><code>4 #ffcd75</code></div>
<div class="swatch"><span style="background:#a7f070"></span><code>5 #a7f070</code></div>
<div class="swatch"><span style="background:#38b764"></span><code>6 #38b764</code></div>
<div class="swatch"><span style="background:#257179"></span><code>7 #257179</code></div>
<div class="swatch"><span style="background:#29366f"></span><code>8 #29366f</code></div>
<div class="swatch"><span style="background:#3b5dc9"></span><code>9 #3b5dc9</code></div>
<div class="swatch"><span style="background:#41a6f6"></span><code>10 #41a6f6</code></div>
<div class="swatch"><span style="background:#73eff7"></span><code>11 #73eff7</code></div>
<div class="swatch"><span style="background:#f4f4f4"></span><code>12 #f4f4f4</code></div>
<div class="swatch"><span style="background:#94b0c2"></span><code>13 #94b0c2</code></div>
<div class="swatch"><span style="background:#566c86"></span><code>14 #566c86</code></div>
<div class="swatch"><span style="background:#333c57"></span><code>15 #333c57</code></div>
</div>

## Picking 4 colours

| Look | Palette | Colours |
|------|---------|---------|
| Meadow (Bee Home) | `[0, 6, 4, 12]` | night, green, yellow, white |
| Warm room | `[0, 3, 4, 13]` | night, orange, yellow, grey-blue |
| Sea | `[8, 10, 11, 12]` | navy, blue, cyan, white |
| Old screen | `[15, 7, 5, 12]` | slate, teal, lime, white |

Tips:

- Use a dark or a light background, not a middle one. Then the other three stand out.
- Keep one bright colour for the things that matter, like you, keys and enemies.
- The text box and counters use colour 0 for the box and colour 3 for the text,
  so make colour 3 easy to read on colour 0.
