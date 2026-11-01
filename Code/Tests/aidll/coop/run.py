"""[TEMP] Runs one coop round with bots on a dedicated server and records how the bots behave.

The same probe runs unchanged on the original and on the rebuilt server, so their logs can be
compared. The runner writes three private files into the game directory (the probe as an admin
module, a server settings file and a map list) and removes them when the run ends.
Uses Python's standard library only.
"""
from pathlib import Path
import argparse
import json
import math
import os
import shutil
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
PRIVATE = 'openbf2_aiprobe'

SETTINGS = '''sv.serverName "OpenBF2 AI probe"
sv.password ""
sv.internet 0
sv.serverIP ""
sv.serverPort {port}
sv.punkBuster 0
sv.maxPlayers 16
sv.numPlayersNeededToStart 0
sv.startDelay 5
sv.endDelay 15
sv.spawnTime 15
sv.manDownTime 15
sv.endOfRoundDelay 15
sv.ticketRatio 100
sv.roundsPerMap 1
sv.timeLimit {time_limit}
sv.scoreLimit 0
sv.soldierFriendlyFire 100
sv.vehicleFriendlyFire 100
sv.soldierSplashFriendlyFire 100
sv.vehicleSplashFriendlyFire 100
sv.tkPunishEnabled 0
sv.votingEnabled 0
sv.gameSpyPort {query_port}
sv.allowNATNegotiation 0
sv.interfaceIP "127.0.0.1"
sv.autoRecord {record}
sv.demoQuality 1
sv.adminScript "{private}"
sv.timeBeforeRestartMap 30
sv.autoBalanceTeam 0
sv.teamRatioPercent 100
sv.coopBotRatio 50
sv.coopBotCount {bots}
sv.coopBotDifficulty {difficulty}
sv.voipEnabled 0
sv.useGlobalRank 0
sv.useGlobalUnlocks 0
'''


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--server', required=True, help='bf2_w32ded.exe to run (original or rebuilt)')
    parser.add_argument('--out', required=True, help='output directory for this run')
    parser.add_argument('--game-dir', default=r'C:\dev\BF2', help='game content root (holds mods/bf2)')
    parser.add_argument('--map', default='strike_at_karkand')
    parser.add_argument('--mode', default='gpm_coop')
    parser.add_argument('--size', type=int, default=16)
    parser.add_argument('--bots', type=int, default=16)
    parser.add_argument('--difficulty', type=int, default=50)
    parser.add_argument('--seconds', type=int, default=240, help='wall-clock length of the run')
    parser.add_argument('--interval', type=float, default=5.0, help='seconds between player samples')
    parser.add_argument('--port', type=int, default=16600, help='game port; 16567 is refused')
    parser.add_argument('--record', action='store_true', help='let the server record a demo (sv.autoRecord)')
    parser.add_argument('--time-limit', type=int, default=0,
                        help='round time limit in seconds; default: longer than the run')
    parser.add_argument('--bots-alone', action='store_true',
                        help='rebuilt server only: let bots spawn with no human (BF2_AI_SPAWN_ALONE)')
    parser.add_argument('--no-redirect', action='store_true',
                        help='run in a hidden console instead of capturing stdout/stderr '
                             '(the original server stalls when its output is redirected)')
    return parser.parse_args()


def probe_source(out, interval):
    source = (HERE / 'aiprobe.py').read_text(encoding='utf-8')
    for old, new in (('OUT_PATH = None', 'OUT_PATH = %r' % str(out / 'probe.tsv')),
                     ('INTERVAL = 5.0', 'INTERVAL = %r' % float(interval))):
        if old not in source:
            raise SystemExit('aiprobe.py lacks the line: ' + old)
        source = source.replace(old, new, 1)
    return source


def install(game, args, out):
    admin = game / 'admin'
    settings_dir = game / 'mods' / 'bf2' / 'Settings'
    if not admin.is_dir() or not settings_dir.is_dir():
        raise SystemExit('game directory lacks admin/ or mods/bf2/Settings: %s' % game)
    files = {
        admin / (PRIVATE + '.py'): probe_source(out, args.interval),
        settings_dir / (PRIVATE + '_settings.con'): SETTINGS.format(
            port=args.port, query_port=args.port + 13350, time_limit=args.time_limit or args.seconds + 60,
            record=1 if args.record else 0, private=PRIVATE, bots=args.bots,
            difficulty=args.difficulty),
        settings_dir / (PRIVATE + '_maplist.con'): 'mapList.append %s %s %d\n' % (args.map, args.mode, args.size),
    }
    for path, text in files.items():
        path.write_text(text, encoding='ascii', newline='\r\n')
    return list(files)


def run_server(game, args, out):
    command =[args.server, '+modPath', 'mods/bf2',
               '+config', 'mods/bf2/Settings/%s_settings.con' % PRIVATE,
               '+mapList', 'mods/bf2/Settings/%s_maplist.con' % PRIVATE,
               '+noStatusMonitor', '1']
    env = dict(os.environ)
    if args.bots_alone:
        env['BF2_AI_SPAWN_ALONE'] = '1'
    if args.no_redirect:
        info = subprocess.STARTUPINFO()
        info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        info.wShowWindow = 0
        process = subprocess.Popen(command, cwd=str(game), env=env, startupinfo=info,
                                   creationflags=subprocess.CREATE_NEW_CONSOLE)
        streams = []
    else:
        stdout = open(out / 'stdout.txt', 'wb')
        stderr = open(out / 'stderr.txt', 'wb')
        process = subprocess.Popen(command, cwd=str(game), env=env, stdout=stdout, stderr=stderr)
        streams = [stdout, stderr]
    start = time.time()
    exited = None
    while time.time() - start < args.seconds:
        code = process.poll()
        if code is not None:
            exited = code
            break
        time.sleep(1)
    elapsed = time.time() - start
    if exited is None:
        process.terminate()
        process.wait(timeout=30)
    for stream in streams:
        stream.close()
    return elapsed, exited


def summarize(out, elapsed, exited):
    rows = []
    probe = out / 'probe.tsv'
    if probe.exists():
        for line in probe.read_text(encoding='latin-1').splitlines():
            rows.append(line.split('\t'))
    bots, last, distance = set(), {}, {}
    alive_at_end, events = {}, {}
    for row in rows:
        kind = row[1] if len(row) > 1 else ''
        events[kind] = events.get(kind, 0) + 1
        if kind != 'P' or len(row) < 18 or row[4] != '1':
            continue
        index, alive = row[2], row[6] == '1'
        bots.add(index)
        alive_at_end[index] = alive
        try:
            position = (float(row[12]), float(row[13]), float(row[14]))
        except ValueError:
            continue
        if alive and index in last:
            distance[index] = distance.get(index, 0.0) + math.dist(position, last[index])
        last[index] = position if alive else None
        if last[index] is None:
            del last[index]
    stop = None
    stderr = out / 'stderr.txt'
    if stderr.exists():
        for line in stderr.read_text(encoding='latin-1').splitlines():
            if '[FRONTIER]' in line or 'FATAL' in line or 'Text:' in line:
                stop = (stop + ' | ' if stop else '') + line.strip()
    summary = {
        'seconds': round(elapsed, 1),
        'server_exited_early': exited is not None,
        'exit_code': exited,
        'stop': stop,
        'bots_seen': len(bots),
        'bots_alive_at_end': sum(1 for alive in alive_at_end.values() if alive),
        'bot_distance_m': round(sum(distance.values()), 1),
        'kills': events.get('KILL', 0),
        'deaths': events.get('DEATH', 0),
        'spawns': events.get('SPAWN', 0),
        'control_point_changes': events.get('CP', 0),
        'vehicle_entries': events.get('ENTER', 0),
        'probe_errors': events.get('ERR', 0),
    }
    (out / 'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    return summary


def main():
    args = parse_args()
    if args.port == 16567:
        raise SystemExit('port 16567 is reserved for interactive testing')
    args.server = str(Path(args.server).resolve())
    game = Path(args.game_dir).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    for old in ('probe.tsv', 'probe_trace.txt', 'stdout.txt', 'stderr.txt', 'summary.json'):
        if (out / old).exists():
            (out / old).unlink()
    installed = install(game, args, out)
    try:
        elapsed, exited = run_server(game, args, out)
    finally:
        for path in installed:
            if path.exists():
                path.unlink()
        trace = game / (PRIVATE + '.log')
        if trace.exists():
            shutil.move(str(trace), str(out / 'probe_trace.txt'))
        cache = game / 'admin' / (PRIVATE + '.pyc')
        if cache.exists():
            cache.unlink()
    summary = summarize(out, elapsed, exited)
    print(json.dumps(summary, indent=2))
    return 1 if exited is not None else 0


if __name__ == '__main__':
    sys.exit(main())
