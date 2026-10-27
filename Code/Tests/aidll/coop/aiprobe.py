# [TEMP] Coop bot observation probe for the original and the rebuilt dedicated servers.
# The server loads it as its admin module (sv.adminScript); run.py installs it under a private
# name and removes it afterwards. Every BF2_AIPROBE_INTERVAL seconds it writes one row per player,
# and it logs connections, spawns, deaths, kills, vehicle entries, control-point captures and game
# status changes as tab-separated rows in OUT_PATH. The game's Python is 2.3, and the original
# server's build has no `os` module, so run.py writes the settings below into the installed copy.
import host
import bf2
from bf2.Timer import Timer

OUT_PATH = None
INTERVAL = 5.0

_out = None
_start = 0.0
_timer = None


def _trace(text):
    # Import, start-up and failure notes, in the server's working directory; run.py collects them.
    try:
        handle = open("openbf2_aiprobe.log", "a")
        handle.write(text + "\n")
        handle.close()
    except Exception:
        pass


_trace("imported, OUT_PATH=%s" % OUT_PATH)


def _log(kind, values):
    if _out is None:
        return
    parts = []
    for value in values:
        parts.append(str(value).replace("\t", " "))
    _out.write("%.1f\t%s\t%s\n" % (host.timer_getWallTime() - _start, kind, "\t".join(parts)))
    _out.flush()


def _template(obj):
    try:
        if obj:
            return obj.templateName
    except Exception:
        pass
    return "-"


def _root(obj):
    try:
        if obj:
            return bf2.objectManager.getRootParent(obj)
    except Exception:
        pass
    return None


def _position(player):
    try:
        soldier = player.getDefaultVehicle()
        if soldier:
            x, y, z = soldier.getPosition()
            return ["%.1f" % x, "%.1f" % y, "%.1f" % z]
    except Exception:
        pass
    return ["-", "-", "-"]


def _index(player):
    if player is None:
        return -1
    return player.index


def _sample(data):
    for player in bf2.playerManager.getPlayers():
        try:
            score = player.score
            row = [player.index, player.getName(), int(bool(player.isAIPlayer())), player.getTeam(),
                   int(bool(player.isAlive())), int(bool(player.isManDown())), player.getSquadId(),
                   int(bool(player.isSquadLeader())), _template(player.getKit()),
                   _template(_root(player.getVehicle()))]
            row = row + _position(player) + [score.score, score.kills, score.deaths]
            _log("P", row)
        except Exception, error:
            _log("ERR", ["sample", _index(player), error])


def onPlayerConnect(player):
    _log("CONNECT", [player.index, player.getName(), int(bool(player.isAIPlayer())), player.getTeam()])


def onPlayerSpawn(player, soldier):
    _log("SPAWN", [player.index, player.getTeam(), _template(player.getKit())] + _position(player))


def onPlayerDeath(player, soldier):
    _log("DEATH", [player.index])


def onPlayerKilled(victim, attacker, weapon, assists, obj):
    _log("KILL", [_index(attacker), _index(victim), _template(weapon)])


def onEnterVehicle(player, vehicle, freeSoldier=False):
    _log("ENTER", [player.index, _template(_root(vehicle))])


def onControlPointChangedOwner(cp, attackingTeam):
    _log("CP", [_template(cp), cp.cp_getParam("team"), attackingTeam])


def onGameStatusChanged(status):
    _log("STATUS", [status, host.sgl_getMapName()])


def init():
    try:
        _init()
        _trace("init done")
    except Exception, error:
        _trace("init failed: %s" % error)


def _init():
    global _out, _start, _timer
    if not OUT_PATH:
        return
    _out = open(OUT_PATH, "a")
    _start = host.timer_getWallTime()
    interval = INTERVAL
    host.registerHandler("PlayerConnect", onPlayerConnect)
    host.registerHandler("PlayerSpawn", onPlayerSpawn)
    host.registerHandler("PlayerDeath", onPlayerDeath)
    host.registerHandler("PlayerKilled", onPlayerKilled)
    host.registerHandler("EnterVehicle", onEnterVehicle)
    host.registerHandler("ControlPointChangedOwner", onControlPointChangedOwner)
    host.registerGameStatusHandler(onGameStatusChanged)
    _timer = Timer(_sample, interval, 1)
    _timer.setRecurring(interval)
    _log("START", [interval])


def shutdown():
    global _out
    _log("SHUTDOWN", [])
    if _out is not None:
        _out.close()
        _out = None


def update():
    pass
