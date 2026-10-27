# Coop bot observation

`run.py` plays one coop round with bots on a dedicated server and records what the bots do.
The probe is an admin module that uses only the stock Python API, so the same run works on the
original `bf2_w32ded.exe` and on the rebuilt one. Comparing the two logs shows how far the rebuilt
AI gets and how close its behavior is. It is a black-box test: it observes bots through the game's
Python interface and does not replace the component fixtures in `aidll/`.

## Run

From the repository root, with stock content in `C:\dev\BF2`:

```powershell
# Original server. It stalls when its output is redirected, so it runs in a hidden console.
python Code/Tests/aidll/coop/run.py --server C:\dev\BF2\bf2_w32ded.exe --no-redirect --out out/aiprobe/original

# Rebuilt server and AI library from a completed x86 build.
python Code/Tests/aidll/coop/run.py --server out/build/<build>/bin/bf2_w32ded.exe --out out/aiprobe/rebuilt
```

The default round is Strike at Karkand, `gpm_coop` 16, with 16 bots for 240 seconds on port 16600
(query port 29950). Use `--map`, `--mode`, `--size`, `--bots`, `--seconds` and `--port` to change
it; port 16567 is refused. `--record` lets the server record a demo of the round.

The runner writes `admin/openbf2_aiprobe.py`, `mods/bf2/Settings/openbf2_aiprobe_settings.con` and
`mods/bf2/Settings/openbf2_aiprobe_maplist.con` into the game directory and removes them when the
run ends. Keep a working copy of the content for these runs, not your playing installation.

## Output

- `probe.tsv`: one row per event, tab-separated: seconds since the probe started, kind, values.
  - `P`: a player sample: index, name, bot, team, alive, man down, squad, squad leader, kit,
    vehicle, x, y, z, score, kills, deaths.
  - `CONNECT`, `SPAWN`, `DEATH`, `KILL` (attacker, victim, weapon), `ENTER`, `CP` (control point,
    new team, attacking team) and `STATUS` (game status, map).
- `summary.json`: run length, whether the server stopped early and the stop text it printed, bots
  seen and alive at the end, total distance the bots moved, and event counts.
- `stdout.txt`, `stderr.txt`: server output when it is not run with `--no-redirect`.

Bot behavior uses random choices and physics, so compare rounds by their patterns (bots alive,
distance per minute, captures, kills) over several runs, not by exact values.

## What the original server does without players

With no human connected, the original server creates the bots, splits them between the teams and
assigns their squads, but keeps every bot on the spawn screen: in a 180-second default run all 16
bots were seen, 8 per team, in `MultiPlayerFreeCamera`, and none spawned. Spawning, movement and
combat need a connected player or a scripted scenario.

The original server's Python has no `os` module, so the probe imports only `host` and `bf2`;
run.py writes the output path and the sample interval into the installed copy.
