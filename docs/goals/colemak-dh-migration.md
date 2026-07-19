# GOAL: Corne ZMK — Colemak-DH + QWERTY toggle, programmer layers, thumbs, RGB, display cleanup

> **Run mode:** hands-free `/goal`. Execute milestones **M0→M9 in order**. Each milestone has a
> **GATE** (commands to run) that is **GREEN** (advance) or **RED** (apply the milestone's
> *Auto-correct*, then re-run the GATE). This is a red→green→auto-correct loop: never advance on RED.
> Make **no pull request**. The goal is DONE when the branch is pushed and the **Build ZMK firmware**
> CI run is green (see M8 / Definition of Done).

---

## Context

Wireless 42-key (6×3+3) Corne, Nice!Nano v2, **upstream ZMK** (`main`), dual displays (an OLED
SSD1306 build **and** a nice!view build are both produced), RGB underglow, ZMK Studio + mouse
enabled. Today it's a Miryoku-style **QWERTY** config (GACS home-row mods, 6 momentary thumb
layer-taps, 14 combos) with a custom **"P" logo** on the right display. First split keyboard for the
user (on a **MacBook** — clipboard/nav use `LG`=⌘).

This goal turns it into a **Colemak-DH-primary** board with QWERTY as a single-key toggle, makes the
symbol/nav layers programmer-friendly (`< >`, grouped brackets, digraph macros), adds thumb combos,
tunes RGB, and **removes the P-logo** custom display code. It also keeps the door open for a future
"zmk-overlay" (live layer on the Mac) — verified constraint: **ZMK Studio's protocol is
keymap-editing only and cannot stream the live layer/keys**, and the split *peripheral has no layer
state*, so live introspection + a right-half layer readout are separate future modules (Prospector
BLE status-advertisement / raw-HID), intentionally **out of scope** here beyond staying non-blocking.

**Decisions baked in** (recommended options; each is localized if you want to flip it later):
live-overlay = *door open only*; RGB = *manual + tuned* (no per-base custom C); right display =
*clean built-in status*; base toggle = *MEDIA-layer key + inner-thumb combo*.

**Repo facts for automation:** owner/repo = `nisargsc/crkbd-zmk`; work branch =
`claude/corne-zmk-config-3n1emf`; firmware build = `.github/workflows/build.yml` ("Build ZMK
firmware", triggers on push touching `config/**` or `build.yaml`, builds 5 targets); SVG =
`.github/workflows/draw-keymap.yml` ("Draw Keymap", regenerates & auto-commits `corne_keymap.svg`).
**There is no local firmware compile** — CI is the compile gate.

---

## Execution protocol (the loop)

```
for M in M0..M9:
    apply M.edits
    loop (max M.max_attempts, default 10):
        run M.GATE
        if GREEN: break
        else: apply the matching row of M.Auto-correct; continue
    if still RED after max_attempts:
        STOP. Report the milestone, the failing GATE output, and what was tried. Do not proceed.
```

**Global stop conditions (report to user, do not thrash):**
- A local GATE stays RED after 10 fix attempts.
- The CI build stays RED after **5** push→fix cycles (M8).
- Any edit would touch a file outside this goal's file list, or require force-pushing over commits
  you did not make.

**Branch safety:** only ever push to `claude/corne-zmk-config-3n1emf`. If `git status` shows the
branch is behind origin because the **Draw Keymap** bot pushed an SVG commit, `git pull --rebase
origin claude/corne-zmk-config-3n1emf` before pushing again. Never `git push --force` here (the
branch carries unmerged work).

**Validation tiers** (defined once, referenced by GATEs):
- **T1 – parse:** `keymap parse -z config/corne.keymap` exits 0 and emits non-empty YAML (syntax).
- **T2 – static validator:** `python3 scratchpad/zmk_validate.py` exits 0 (structure/counts/refs).
- **T3 – config hygiene:** the shell assertion block in M6 exits 0 (display cleanup complete).
- **T4 – CI build:** "Build ZMK firmware" run for the pushed SHA concludes `success` (all 5 targets).
- **T5 – CI draw:** "Draw Keymap" run concludes `success`.

---

## M0 — Setup & validators

**Edits / actions**
1. Confirm branch: `git rev-parse --abbrev-ref HEAD` → must be `claude/corne-zmk-config-3n1emf`
   (if not: `git checkout claude/corne-zmk-config-3n1emf`).
2. Install the parser: `pip install --quiet keymap-drawer` (provides the `keymap` CLI).
3. Write the static validator to `scratchpad/zmk_validate.py` **verbatim from Appendix B**.

**GATE**
```
git rev-parse --abbrev-ref HEAD            # == claude/corne-zmk-config-3n1emf
keymap --version                           # prints a version (parser installed)
python3 -c "import ast,pathlib; ast.parse(pathlib.Path('scratchpad/zmk_validate.py').read_text())"
```
**GREEN when** branch is correct, `keymap` runs, and the validator is syntactically valid Python.

**Auto-correct**
| RED signal | Fix |
|---|---|
| wrong branch | `git checkout claude/corne-zmk-config-3n1emf` (create with `-b` if missing) |
| `keymap: command not found` | retry `pip install keymap-drawer`; if pip network-blocked, skip T1 for the rest of the run and rely on T2 + CI (note this in the final report) |
| validator won't parse | re-copy Appendix B exactly |

---

## M1 — Rewrite the keymap (Colemak-DH base, QWERTY, toggle, SYM rework, macros, combos)

**Edits**
- Overwrite `config/corne.keymap` **verbatim from Appendix A**. This single artifact contains: the
  8 `#define`s + matching node order, Colemak-DH layer 0, QWERTY layer 1 (alphas+HRM real, outer
  keys + thumbs `&trans`), `&tog QWERTY` on MEDIA, the reworked SYM layer,
  6 digraph macros, and 2 new combos (apostrophe, inner-thumb base toggle).

**GATE** = **T1** then **T2**:
```
keymap parse -z config/corne.keymap > scratchpad/km.yaml && test -s scratchpad/km.yaml
python3 scratchpad/zmk_validate.py
```
**GREEN when** both exit 0. T2 prints `GREEN — 8 layers x 42 keys, defines/order/refs OK`.

**Auto-correct**
| RED signal (from T1/T2) | Fix |
|---|---|
| `[<node>] N bindings, expected 42` | that layer's row is miscounted — recount against Appendix A; every layer is 12+12+12+6 |
| `[order] layer nodes … != expected` | node block order must be colemak,qwerty,nav,num,media,sym,fun,mouse |
| `[defines] X should be N` | fix the `#define` value; indices are 0..7 in that order |
| `[<node>] unknown behavior &X` | macro label/reference mismatch — ensure `&arrow &fatarrow &eqeq &neq &logand &logor &hyper` are defined in `macros{}` and spelled identically where used |
| `[ref] &tog QWERTY not a defined layer` | keep `#define QWERTY 1` present |
| `display-name '…' > 9 chars` | shorten (OLED buffer); all names in Appendix A are ≤7 |
| T1 parse error at line N | open that line — usually an unbalanced `< >`, a missing `;`, or a stray brace; fix and re-run |

---

## M2 — RGB startup tuning (config-only)

**Edits** — in `config/corne.conf`, under the `# RGB config` block, **add** these lines (keep the
existing `EFF_START=3`, `BRT_MAX=60`, `AUTO_OFF_IDLE=y`, `EXT_POWER=n`):
```
CONFIG_ZMK_RGB_UNDERGLOW_HUE_START=210
CONFIG_ZMK_RGB_UNDERGLOW_SAT_START=100
CONFIG_ZMK_RGB_UNDERGLOW_BRT_START=60
```
(Sets a calm blue default for the Solid/Breathe effects; ignored under Swirl but harmless. Manual
`&rgb_ug` controls already live on MEDIA — unchanged.)

**GATE**
```
grep -q "CONFIG_ZMK_RGB_UNDERGLOW_HUE_START=210" config/corne.conf
grep -c "CONFIG_ZMK_RGB_UNDERGLOW" config/corne.conf     # >= 7 lines now
```
**GREEN when** the new key is present and the RGB block is intact.

**Auto-correct**
| RED signal | Fix |
|---|---|
| key missing | re-add the three lines; do not remove existing RGB lines |
| accidentally duplicated a symbol | keep one definition per symbol (last wins in Kconfig, but keep it clean) |

---

## M3 — Remove the "P" logo custom display (files)

**Edits**
1. Delete `config/boards/shields/corne/src/custom_status_screen.c`.
2. Delete `config/boards/shields/corne/custom_config.h` (vestigial QMK leftover).
3. Overwrite `config/boards/shields/corne/CMakeLists.txt` **verbatim from Appendix C-1**.
4. Overwrite `config/boards/shields/corne/Kconfig.defconfig` **verbatim from Appendix C-2**
   (identical to today minus the `config CORNE_CUSTOM_DISPLAY` stanza).
5. Overwrite `config/boards/shields/corne/corne_right.conf` **verbatim from Appendix C-3**
   (RGB + `CONFIG_ZMK_DISPLAY=y` only; all custom-screen / LVGL / widget lines removed).

**GATE** = **T3** (see M6 for the full block; run it now too):
```
test ! -e config/boards/shields/corne/src/custom_status_screen.c
test ! -e config/boards/shields/corne/custom_config.h
! grep -rIn "CORNE_CUSTOM_DISPLAY\|STATUS_SCREEN_CUSTOM\|custom_status_screen\|NICE_VIEW_WIDGET_STATUS" config/
grep -q "CONFIG_ZMK_DISPLAY=y" config/boards/shields/corne/corne_right.conf
```
**GREEN when** all four commands succeed (the `grep` line finds nothing → exits non-zero → negated to
success).

**Auto-correct**
| RED signal | Fix |
|---|---|
| file still present | delete it (`git rm` or `rm`) |
| dangling reference found | open the printed file:line and remove the reference (must be gone from `corne_right.conf`, `Kconfig.defconfig`, `CMakeLists.txt`) |
| right display disabled | ensure `corne_right.conf` keeps `CONFIG_ZMK_DISPLAY=y` (do **not** add `STATUS_SCREEN_BUILT_IN` there — defaults give OLED→built-in, nice!view→art) |

> Do **not** modify `corne_left.conf` — leaving its `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y`
> keeps the base name (COLEMAK/QWERTY) on the left half for both build variants.

---

## M4 — Docs: README + keymap header

**Edits**
1. `README.md` — replace the **## Display** section body with:
   ```
   - **Left (central):** Built-in ZMK status screen — active base (COLEMAK / QWERTY) or held layer, battery, BT
   - **Right (peripheral):** Built-in status — battery + connection (OLED build) / nice!view art widget (nice!view build)
   ```
   Update the intro line to mention **"Colemak-DH primary with a single-key QWERTY toggle"**, and in
   **## Interactive Viewer** change `Press 0-6` → `Press 0-7`.
2. The keymap header comment is already correct in Appendix A (8-layer list) — no extra edit.

**GATE**
```
! grep -in "P keycap\|P logo\|glitch" README.md
grep -q "0-7" README.md
grep -qi "colemak" README.md
```
**GREEN when** no P-logo wording remains and the new strings are present.

**Auto-correct**
| RED signal | Fix |
|---|---|
| P-logo wording remains | remove/replace it in README |
| `0-7` missing | update the viewer note |

---

## M5 — Local green gate (all static tiers together)

**GATE** — re-run everything to confirm nothing regressed across M1–M4:
```
keymap parse -z config/corne.keymap > scratchpad/km.yaml && test -s scratchpad/km.yaml   # T1
python3 scratchpad/zmk_validate.py                                                         # T2
bash scratchpad/hygiene.sh                                                                 # T3 (Appendix C-4)
git diff --stat                                                                            # only intended files
```
**GREEN when** T1+T2+T3 pass and `git diff --stat` lists **only**: `config/corne.keymap`,
`config/corne.conf`, `config/boards/shields/corne/corne_right.conf`,
`config/boards/shields/corne/Kconfig.defconfig`, `config/boards/shields/corne/CMakeLists.txt`,
`README.md`, and the two deletions under `config/boards/shields/corne/`.

**Auto-correct:** if an unexpected file appears in the diff, revert it (`git checkout -- <file>`); if
a tier is RED, return to the owning milestone's auto-correct table.

---

## M6 — Commit & push

**Actions**
```
git add -A
git commit -m "feat(keymap): Colemak-DH base + QWERTY toggle, programmer layers, thumb combos, RGB tune, drop P-logo display

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_018V76PG29Z6dRoSREQXyjUF"
git push -u origin claude/corne-zmk-config-3n1emf
```
Retry push on network error only, backoff 2s/4s/8s/16s (max 4).

**GATE**
```
git log origin/claude/corne-zmk-config-3n1emf -1 --oneline    # shows the new commit
git status -sb                                                  # clean, not ahead
```
**GREEN when** the commit is on origin and the tree is clean.

**Auto-correct**
| RED signal | Fix |
|---|---|
| non-fast-forward (Draw bot pushed) | `git pull --rebase origin claude/corne-zmk-config-3n1emf` then push again |
| network error | backoff-retry per above |
| nothing to commit | a prior run already committed — proceed to M7 |

---

## M7 — Load CI tooling

**Action:** the GitHub MCP tools are deferred — load their schemas before use:
```
ToolSearch "select:mcp__github__actions_list,mcp__github__actions_get,mcp__github__get_job_logs"
```
(Also `mcp__github__get_commit` if you need to resolve the pushed SHA: `git rev-parse HEAD`.)

**GATE:** the three tool schemas are returned (callable).
**Auto-correct:** if the github MCP server is disconnected, `ToolSearch "github actions logs"` to
re-discover; if still unavailable, STOP and report that CI must be checked manually (give the branch
name + the local green status so the user can watch Actions themselves).

---

## M8 — CI build loop (Build ZMK firmware → green)   [outer red/green loop, max 5 cycles]

**Poll:** using `owner=nisargsc, repo=crkbd-zmk`:
1. `mcp__github__actions_list` → find the most recent **"Build ZMK firmware"** run whose head SHA ==
   `git rev-parse HEAD`. (Push touches `config/**`, so it is triggered automatically.)
2. `mcp__github__actions_get` on that run id → read `status`/`conclusion`. If `status` != `completed`,
   wait and re-check (the run takes several minutes; re-poll rather than sleeping in the foreground).
3. On `conclusion == success` → **GREEN**, go to M9.
4. On `conclusion == failure` → `mcp__github__get_job_logs` (failed jobs only) → map the error via the
   table below → apply the fix → **re-run M5 (local green) → M6 (push)** → return to step 1.
   Count this as one cycle; after 5 failed cycles, STOP and report the last log excerpt.

**Auto-correct — common ZMK build failures → fix**
| Log signature | Likely cause | Fix |
|---|---|---|
| `undefined node label 'X'` / `'X' undeclared` in `.keymap` region | macro label typo or a reference to a macro not defined | ensure `&arrow/&fatarrow/&eqeq/&neq/&logand/&logor` labels exist in `macros{}` and match uses; re-run T2 |
| `<behavior> ... #binding-cells` mismatch | wrong arg count (e.g. `&mt` needs 2, `&kp` needs 1, `&tog` needs 1) | correct the offending binding in the layer |
| `keymap` / `matrix transform` size / "expected 42" | a layer isn't 42 keys | T2 should have caught it — recount that layer |
| unknown keycode (e.g. `LA(...)`, `GT`, `PIPE`) | typo vs ZMK `keys.h` | verify the exact keycode name; all names used are standard ZMK |
| CMake `Cannot find source file .../custom_status_screen.c` | CMakeLists still references the deleted file | apply Appendix C-1 exactly |
| Kconfig `warning: unmet direct dependencies` / missing `CORNE_CUSTOM_DISPLAY` | leftover reference to the removed symbol | grep-clean per M3/T3 |
| LVGL / display driver error on a build target | a needed display CONFIG was dropped | restore `CONFIG_ZMK_DISPLAY=y` in the affected `.conf`; do not touch `corne_left.conf` |
| only the `nice_view` targets fail | display-choice defaults | confirm you did **not** add `STATUS_SCREEN_*` to `corne_right.conf`; let defaults apply |
| combo error `too many combos for key` | a position exceeds `MAX_COMBOS_PER_KEY` (default 5) | none expected (max is 3); if hit, add `CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY=6` to `config/corne.conf` |

---

## M9 — Draw workflow & finalize

1. `mcp__github__actions_list` → confirm the latest **"Draw Keymap"** run for HEAD concluded
   `success` (**T5**). It auto-commits `corne_keymap.svg`; if it pushed, `git pull --rebase` locally.
2. Optionally open **ZMK Studio** over USB later to confirm the live keymap — manual, not gating.
3. Emit the final report (see Definition of Done).

**Auto-correct:** if Draw Keymap failed, read its log — usually a keymap-drawer parse issue already
caught by T1; fix, push, and it re-runs. A red Draw run does **not** block firmware (they're
independent), but note it in the report.

---

## Definition of Done (report all of these)

- [ ] **T1** parse GREEN, **T2** validator GREEN (8×42, order, refs), **T3** hygiene GREEN.
- [ ] Commit pushed to `claude/corne-zmk-config-3n1emf`; only the intended files changed.
- [ ] **T4** "Build ZMK firmware" GREEN for all 5 targets (both OLED halves, both nice!view halves,
      settings_reset).
- [ ] **T5** "Draw Keymap" GREEN; `corne_keymap.svg` shows COLEMAK + QWERTY + 6 feature layers.
- [ ] Report: what changed, the CI run URL/id, and the **manual hardware checklist** below.
- [ ] **No PR created.**

If any box can't be checked after the iteration caps, STOP and report exactly where and why.

## Manual hardware checklist (human, post-flash — cannot be automated)

Flash the `.uf2` for your display variant to each half, then verify:
- Left display reads **COLEMAK** at rest; the MEDIA-layer toggle (hold left-thumb ESC → top-left key)
  **and** the inner-thumb combo (both inner thumbs together) each flip it to **QWERTY** and back;
  holding a feature thumb briefly shows that layer's name.
- Colemak-DH letters + GACS home-row mods on both hands type correctly; the apostrophe combo (Colemak
  J+L, positions 6+7) emits `'`.
- SYM (hold right-thumb ENTER): `( ) < > = { } [ ]` and digraphs `-> => == != && ||` all emit; left
  thumbs give `- _ +`.
- NAV (hold left-thumb SPACE): arrows work; word/line motion via ⌥/⌘ + arrow (hold NAV's home-row
  Alt/Cmd, tap an arrow); existing combos fire.
- Right display shows clean status (no P logo); RGB responds to the MEDIA controls.

---

# Appendix A — full `config/corne.keymap` (write verbatim)

```dts
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

//
//   Colemak-DH primary base + QWERTY alternate (single-key toggle).
//   Miryoku-style: GACS home-row mods, 6 thumb layer-taps, vim arrows.
//
//   Layer 0: COLEMAK — Colemak-DH + home row mods (default base)
//   Layer 1: QWERTY  — QWERTY alternate base (toggle: MEDIA key or inner-thumb combo)
//   Layer 2: NAV     — vim arrows, clipboard, nav (hold SPACE)
//   Layer 3: NUM     — numpad left hand (hold BSPC)
//   Layer 4: MEDIA   — media, BT, RGB, base toggle (hold ESC)
//   Layer 5: SYM     — symbols left hand, digraphs right hand (hold ENTER)
//   Layer 6: FUN     — function keys left hand (hold DEL)
//   Layer 7: MOUSE   — mouse move/scroll/buttons (hold TAB)
//

#include <behaviors.dtsi>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/ext_power.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/outputs.h>
#include <dt-bindings/zmk/pointing.h>
#include <dt-bindings/zmk/rgb.h>

#define COLEMAK 0
#define QWERTY  1
#define NAV     2
#define NUM     3
#define MEDIA   4
#define SYM     5
#define FUN     6
#define MOUSE   7

// Corne key positions (42 keys):
// Row 0:  0  1  2  3  4  5    6  7  8  9  10  11
// Row 1: 12 13 14 15 16 17   18 19 20 21  22  23
// Row 2: 24 25 26 27 28 29   30 31 32 33  34  35
// Thumb:          36 37 38   39 40 41

&mt {
    flavor = "tap-preferred";
    tapping-term-ms = <200>;
    quick-tap-ms = <175>;
    require-prior-idle-ms = <150>;
};

&lt {
    flavor = "balanced";
    tapping-term-ms = <200>;
    quick-tap-ms = <175>;
};

/ {
    macros {
        hyper: hyper {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings =
                <&macro_press &kp LGUI &kp LALT &kp LCTRL &kp LSHFT>,
                <&macro_pause_for_release>,
                <&macro_release &kp LGUI &kp LALT &kp LCTRL &kp LSHFT>;
        };

        arrow: arrow {          // ->
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp MINUS &kp GT>;
        };
        fatarrow: fatarrow {    // =>
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp EQUAL &kp GT>;
        };
        eqeq: eqeq {            // ==
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp EQUAL &kp EQUAL>;
        };
        neq: neq {              // !=
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp EXCL &kp EQUAL>;
        };
        logand: logand {        // &&
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp AMPS &kp AMPS>;
        };
        logor: logor {          // ||
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp PIPE &kp PIPE>;
        };
    };

    combos {
        compatible = "zmk,combos";

        // -- Tier 1: Essential --
        combo_caps_word { bindings = <&caps_word>; key-positions = <16 19>; timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_esc  { bindings = <&kp ESC>;  key-positions = <2 3>;  timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_bspc { bindings = <&kp BSPC>; key-positions = <8 9>;  timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_del  { bindings = <&kp DEL>;  key-positions = <9 10>; timeout-ms = <30>; require-prior-idle-ms = <150>; };

        // -- Tier 2: High Value --
        combo_tab   { bindings = <&kp TAB>;   key-positions = <14 15>; timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_enter { bindings = <&kp RET>;   key-positions = <20 21>; timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_copy  { bindings = <&kp LG(C)>; key-positions = <26 27>; timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_paste { bindings = <&kp LG(V)>; key-positions = <27 28>; timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_cut   { bindings = <&kp LG(X)>; key-positions = <26 28>; timeout-ms = <30>; require-prior-idle-ms = <150>; };

        // -- Tier 3: Vertical symbol combos --
        combo_minus      { bindings = <&kp MINUS>; key-positions = <19 31>; timeout-ms = <40>; require-prior-idle-ms = <100>; };
        combo_underscore { bindings = <&kp UNDER>; key-positions = <18 30>; timeout-ms = <40>; require-prior-idle-ms = <100>; };
        combo_equal      { bindings = <&kp EQUAL>; key-positions = <16 28>; timeout-ms = <40>; require-prior-idle-ms = <100>; };
        combo_grave      { bindings = <&kp GRAVE>; key-positions = <14 26>; timeout-ms = <40>; require-prior-idle-ms = <100>; };
        combo_semi       { bindings = <&kp SEMI>;  key-positions = <21 22>; timeout-ms = <30>; require-prior-idle-ms = <150>; };

        // -- New --
        combo_apostrophe  { bindings = <&kp SQT>;      key-positions = <6 7>;   timeout-ms = <30>; require-prior-idle-ms = <150>; };
        combo_base_toggle { bindings = <&tog QWERTY>;  key-positions = <38 39>; timeout-ms = <50>; require-prior-idle-ms = <50>; };
    };

    keymap {
        compatible = "zmk,keymap";

        colemak_layer {
            display-name = "COLEMAK";
            bindings = <
&kp TAB        &kp Q       &kp W       &kp F        &kp P        &kp B      &kp J        &kp L        &kp U        &kp Y       &kp SEMI     &kp BSPC
&mt LCTRL ESC  &mt LGUI A  &mt LALT R  &mt LCTRL S  &mt LSHFT T  &kp G      &kp M        &mt RSHFT N  &mt RCTRL E  &mt RALT I  &mt RGUI O   &hyper
&kp LSHFT      &kp Z       &kp X       &kp C        &kp D        &kp V      &kp K        &kp H        &kp COMMA    &kp DOT     &kp FSLH     &kp ESC
                                       &lt MEDIA ESC &lt NAV SPACE &lt MOUSE TAB  &lt SYM RET &lt NUM BSPC  &lt FUN DEL
            >;
        };

        qwerty_layer {
            display-name = "QWERTY";
            bindings = <
&trans         &kp Q       &kp W       &kp E        &kp R        &kp T      &kp Y        &kp U         &kp I         &kp O       &kp P        &trans
&trans         &mt LGUI A  &mt LALT S  &mt LCTRL D  &mt LSHFT F  &kp G      &kp H        &mt RSHFT J   &mt RCTRL K   &mt RALT L  &mt RGUI SQT &trans
&trans         &kp Z       &kp X       &kp C        &kp V        &kp B      &kp N        &kp M         &kp COMMA     &kp DOT     &kp FSLH     &trans
                                       &trans        &trans        &trans     &trans       &trans        &trans
            >;
        };

        nav_layer {
            display-name = "NAV";
            bindings = <
&none          &none       &none       &none         &none         &none          &kp LG(LS(Z)) &kp LG(V)   &kp LG(C)     &kp LG(X)    &kp LG(Z)    &none
&none          &kp LGUI    &kp LALT    &kp LCTRL     &kp LSHFT     &none          &kp LEFT    &kp DOWN      &kp UP        &kp RIGHT    &kp CAPS     &none
&none          &none       &none       &none         &none         &none          &kp INS     &kp HOME      &kp PG_DN     &kp PG_UP    &kp END      &none
                                       &none         &none         &none          &kp RET     &kp BSPC      &kp DEL
            >;
        };

        num_layer {
            display-name = "NUM";
            bindings = <
&none          &kp LBKT    &kp N7      &kp N8        &kp N9        &kp RBKT       &none       &none         &none         &none        &none        &none
&none          &kp SEMI    &kp N4      &kp N5        &kp N6        &kp EQUAL      &none       &kp RSHFT     &kp RCTRL     &kp RALT     &kp RGUI     &none
&none          &kp GRAVE   &kp N1      &kp N2        &kp N3        &kp BSLH       &none       &none         &none         &none        &none        &none
                                       &kp DOT       &kp N0        &kp MINUS      &none       &none         &none
            >;
        };

        media_layer {
            display-name = "MEDIA";
            bindings = <
&none          &tog QWERTY &none       &none         &none         &none          &rgb_ug RGB_TOG   &rgb_ug RGB_EFF &rgb_ug RGB_HUI &rgb_ug RGB_SAI &none      &none
&none          &kp LGUI    &kp LALT    &kp LCTRL     &kp LSHFT     &none          &ext_power EP_TOG &kp C_PREV      &kp C_VOL_DN    &kp C_VOL_UP    &kp C_NEXT &none
&none          &none       &none       &none         &none         &none          &out OUT_TOG      &bt BT_SEL 0    &bt BT_SEL 1    &bt BT_SEL 2    &bt BT_SEL 3 &bt BT_CLR
                                       &none         &none         &none          &kp C_STOP        &kp C_PP        &kp C_MUTE
            >;
        };

        sym_layer {
            display-name = "SYM";
            bindings = <
&none          &kp LBRC    &kp RBRC    &kp LBKT      &kp RBKT      &kp CARET      &arrow      &fatarrow     &eqeq         &neq         &kp PIPE     &none
&none          &kp LPAR    &kp RPAR    &kp LT        &kp GT        &kp EQUAL      &none       &kp RSHFT     &kp RCTRL     &kp RALT     &kp RGUI     &none
&none          &kp TILDE   &kp GRAVE   &kp PIPE      &kp AMPS      &kp EXCL       &logand     &logor        &none         &none        &none        &none
                                       &kp MINUS     &kp UNDER     &kp PLUS       &none       &none         &none
            >;
        };

        fun_layer {
            display-name = "FUN";
            bindings = <
&none          &kp F12     &kp F7      &kp F8        &kp F9        &kp PSCRN      &none       &none         &none         &none        &none        &none
&none          &kp F11     &kp F4      &kp F5        &kp F6        &kp SLCK       &none       &kp RSHFT     &kp RCTRL     &kp RALT     &kp RGUI     &none
&none          &kp F10     &kp F1      &kp F2        &kp F3        &kp PAUSE_BREAK &none      &none         &none         &none        &none        &none
                                       &kp K_APP     &kp SPACE     &kp TAB        &none       &none         &none
            >;
        };

        mouse_layer {
            display-name = "MOUSE";
            bindings = <
&none          &none       &none       &none         &none         &none          &none            &none            &none          &none             &none        &none
&none          &kp LGUI    &kp LALT    &kp LCTRL     &kp LSHFT     &none          &mmv MOVE_LEFT   &mmv MOVE_DOWN   &mmv MOVE_UP   &mmv MOVE_RIGHT   &none        &none
&none          &none       &none       &none         &none         &none          &msc SCRL_LEFT   &msc SCRL_DOWN   &msc SCRL_UP   &msc SCRL_RIGHT   &none        &none
                                       &none         &none         &none          &mkp MCLK        &mkp LCLK        &mkp RCLK
            >;
        };
    };
};
```

---

# Appendix B — `scratchpad/zmk_validate.py` (write verbatim)

```python
#!/usr/bin/env python3
"""Static validator for config/corne.keymap. Run from repo root. Exit 0 = GREEN."""
import re, sys, pathlib

KEYMAP = pathlib.Path("config/corne.keymap")
EXPECTED = ["COLEMAK", "QWERTY", "NAV", "NUM", "MEDIA", "SYM", "FUN", "MOUSE"]
NODES = [n.lower() + "_layer" for n in EXPECTED]
KEYS, MAXNAME = 42, 9
BUILTINS = {"kp", "mt", "lt", "mo", "to", "tog", "trans", "none", "caps_word",
            "key_repeat", "rgb_ug", "ext_power", "out", "bt", "mmv", "msc", "mkp",
            "sk", "sl", "gresc", "bootloader", "sys_reset", "studio_unlock",
            "macro_press", "macro_release", "macro_pause_for_release"}

errs = []
src = KEYMAP.read_text()
code = re.sub(r'//[^\n]*', '', re.sub(r'/\*.*?\*/', '', src, flags=re.S))

# 1) #define layer indices
defs = {m.group(1): int(m.group(2)) for m in re.finditer(r'#define\s+([A-Z_]+)\s+(\d+)', src)}
for i, name in enumerate(EXPECTED):
    if defs.get(name) != i:
        errs.append(f"[defines] {name} should be {i}, got {defs.get(name)}")

# 2) keymap block + node order
kmtext = code[code.index("keymap"):] if "keymap" in code else ""
found = re.findall(r'(\w+_layer)\s*\{', kmtext)
if found != NODES:
    errs.append(f"[order] layer nodes {found} != expected {NODES}")

# 3) macros defined
macro_labels = set(re.findall(r'(\w+):\s*\w+\s*\{\s*compatible\s*=\s*"zmk,behavior-macro"', code))
known = BUILTINS | macro_labels

# 4) per-layer: display-name length, 42 bindings, known behavior refs
for node in NODES:
    m = re.search(node + r'\s*\{([^}]*)\}', kmtext, re.S)
    if not m:
        errs.append(f"[{node}] node not found"); continue
    body = m.group(1)
    dn = re.search(r'display-name\s*=\s*"([^"]*)"', body)
    if not dn:
        errs.append(f"[{node}] missing display-name")
    elif len(dn.group(1)) > MAXNAME:
        errs.append(f"[{node}] display-name '{dn.group(1)}' > {MAXNAME} chars")
    b = re.search(r'bindings\s*=\s*<(.*?)>\s*;', body, re.S)
    if not b:
        errs.append(f"[{node}] missing bindings"); continue
    n = b.group(1).count("&")
    if n != KEYS:
        errs.append(f"[{node}] {n} bindings, expected {KEYS}")
    for ref in re.findall(r'&(\w+)', b.group(1)):
        if ref not in known:
            errs.append(f"[{node}] unknown behavior &{ref}")

# 5) layer references
for beh, arg in re.findall(r'&(lt|mo|to|tog)\s+([A-Z_]+|\d+)', code):
    if arg.isdigit():
        if int(arg) >= len(EXPECTED):
            errs.append(f"[ref] &{beh} {arg} out of range")
    elif arg not in defs:
        errs.append(f"[ref] &{beh} {arg} not a defined layer")

# 6) combo positions in 0..41
for grp in re.findall(r'key-positions\s*=\s*<([\d\s]+)>', code):
    for p in grp.split():
        if not (0 <= int(p) <= 41):
            errs.append(f"[combo] position {p} out of 0..41")

if errs:
    print("RED — keymap validation failed:")
    for e in errs:
        print("  -", e)
    sys.exit(1)
print(f"GREEN — {len(NODES)} layers x {KEYS} keys, defines/order/refs OK")
```

---

# Appendix C — other file targets

### C-1  `config/boards/shields/corne/CMakeLists.txt` (write verbatim)
```cmake
# Corne shield: no custom C sources (built-in status screens used on both halves).
```

### C-2  `config/boards/shields/corne/Kconfig.defconfig` (write verbatim)
```
if SHIELD_CORNE_LEFT

config ZMK_KEYBOARD_NAME
	default "Corne"

config ZMK_SPLIT_ROLE_CENTRAL
	default y

endif

if SHIELD_CORNE_LEFT || SHIELD_CORNE_RIGHT

config ZMK_SPLIT
	default y

if ZMK_DISPLAY

config I2C
	default y

config SSD1306
	default y

endif # ZMK_DISPLAY

if LVGL

config LV_Z_VDB_SIZE
	default 64

config LV_DPI_DEF
	default 148

config LV_Z_BITS_PER_PIXEL
	default 1

choice LV_COLOR_DEPTH
	default LV_COLOR_DEPTH_1
endchoice

endif # LVGL

endif
```

### C-3  `config/boards/shields/corne/corne_right.conf` (write verbatim)
```
# RGB config
CONFIG_ZMK_RGB_UNDERGLOW=y
# Turn off RGB when keyboard enters idle
CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE=y

# Display — built-in status screen.
# OLED build  -> peripheral status (connection + battery).
# nice!view build -> nice!view art widget (battery + animation) by shield default.
CONFIG_ZMK_DISPLAY=y
```

### C-4  `scratchpad/hygiene.sh` (write verbatim; used by T3 / M5)
```bash
#!/usr/bin/env bash
set -euo pipefail
test ! -e config/boards/shields/corne/src/custom_status_screen.c
test ! -e config/boards/shields/corne/custom_config.h
if grep -rIn "CORNE_CUSTOM_DISPLAY\|STATUS_SCREEN_CUSTOM\|custom_status_screen\|NICE_VIEW_WIDGET_STATUS" config/ ; then
  echo "RED: dangling custom-display references"; exit 1
fi
grep -q "CONFIG_ZMK_DISPLAY=y" config/boards/shields/corne/corne_right.conf
echo "GREEN: display cleanup verified"
```

---

## Notes / rationale (for the operator, not executed)

- **Toggle = `&tog QWERTY`** (verified ZMK semantics): one binding flips QWERTY on/off atop the
  always-on Colemak default; QWERTY defines its own alphas so it shadows Colemak, and `&trans` on
  QWERTY's outer keys + thumbs falls through to Colemak (defined once). The **left** built-in
  layer-status widget renders the highest active layer's `display-name` as text → COLEMAK/QWERTY with
  zero custom code.
- **Colemak-DH** = Mod-DH matrix variant; GACS mods land on `A R S T` / `N E I O`, inner-index
  `G`/`M` unmodded (same feel as today's `G`/`H`). `'` loses its base home → `combo_apostrophe`.
- **Combos stay global/position-based** (they emit fixed keycodes, so they behave the same on both
  bases). 16 combos total; max combos-per-key = 3 (< default 5), so no Kconfig bump needed.
- **Right display can't show the layer** (peripheral has no layer state) — that, live keypress/layer
  streaming to the Mac (Prospector BLE status-advertisement or `zzeneg/zmk-raw-hid`), and per-layer
  RGB color (custom `zmk_layer_state_changed` listener) are deliberately deferred as separate modules.
```

---

## ✅ EXECUTION OUTCOME (2026-07-19, hands-free run)

**Status: COMPLETE — firmware built and delivered.**

- Static gates GREEN: **T2** validator (8 layers × 42 keys, defines/order/refs OK) and **T3** hygiene
  (P-logo removed cleanly). **T1** (keymap-drawer parse) was skipped — a broken tree-sitter/keymap-drawer
  combo in the run environment — and is superseded by a real compile below.
- Commit `cd337e4` pushed to `claude/corne-zmk-config-3n1emf`.
- **GitHub Actions is DISABLED on this repository.** `list_workflows`, `get_workflow build.yml`, and a
  `workflow_dispatch` all return 404, and there are zero runs ever. The intended CI compile (T4/T5)
  could not run. Re-enable at **Settings → Actions → General** to restore automatic `.uf2` builds and
  the keymap-SVG workflow.
- **Fallback compile (real gate): built ZMK locally** in the run container — `west` + Zephyr +
  the Debian `gnuarmemb` ARM toolchain (the Zephyr SDK host was blocked by the network proxy).
  **All 5 targets compiled error-free**, proving the keymap + config are valid firmware:
  `corne_left`/`corne_right` (OLED) and both `nice_view` variants, plus `settings_reset`.
  Board target: `nice_nano//zmk` (ZMK Hardware-Model-v2).
- The five `.uf2` binaries were delivered to the user via chat (not committed — build artifacts don't
  belong in the config repo). Regenerate them via CI once Actions is enabled, or locally with the
  same west workspace.
