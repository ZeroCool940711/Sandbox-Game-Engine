import sys, os
import dungeonGenerator, random, datetime
import graphicEngine_PIL
from time import time
from datetime import timedelta

# method to format time function
def secondsToText(secs, lang="EN"):
    days = secs//86400
    hours = (secs - days*86400)//3600
    minutes = (secs - days*86400 - hours*3600)//60
    seconds = secs - days*86400 - hours*3600 - minutes*60

    if lang == "ES":
        days_text = "día{}".format("s" if days!=1 else "")
        hours_text = "hora{}".format("s" if hours!=1 else "")
        minutes_text = "minuto{}".format("s" if minutes!=1 else "")
        seconds_text = "segundo{}".format("s" if seconds!=1 else "")
    elif lang == "DE":
        days_text = "Tag{}".format("e" if days!=1 else "")
        hours_text = "Stunde{}".format("n" if hours!=1 else "")
        minutes_text = "Minute{}".format("n" if minutes!=1 else "")
        seconds_text = "Sekunde{}".format("n" if seconds!=1 else "")
    else:
        #Default to English
        days_text = "day{}".format("s" if days!=1 else "")
        hours_text = "hour{}".format("s" if hours!=1 else "")
        minutes_text = "minute{}".format("s" if minutes!=1 else "")
        seconds_text = "second{}".format("s" if seconds!=1 else "")

    result = ", ".join(filter(lambda x: bool(x),[
        "{0} {1}".format(days, days_text) if days else "",
        "{0} {1}".format(hours, hours_text) if hours else "",
        "{0} {1}".format(minutes, minutes_text) if minutes else "",
        "{0} {1}".format(seconds, seconds_text) if seconds else ""
    ]))
    return result

# define map size (x,y) max tiles to use in direction
levelSize = [64,64]

# we use the time module to check how long it takes for the script to run
# this way we can know if the script is faster or not when we add something.
start_time = time()

# create class instance; ALWAYS required
d = dungeonGenerator.dungeonGenerator(levelSize[0], levelSize[1])

# generate room areas
d.placeRandomRooms(7, 9, 2, 1, 1000)
# build corridors in empty spaces
d.generateCorridors('m')
# connect paths so rooms are accessable via door
d.connectAllRooms(10)
# now find and connect unconnected areas via merge
d.mergeUnconnectedAreas()
# purge deadends; not required but makes things cleaner looking
d.pruneDeadends(10)
# always call place walls after basic structure is made
d.placeWalls()



# Overlay Extention Example
# ---------------------------------
print("Map structure made; generating defined overlays")

# make call to find/define hostable tiles for use later on; this must be called to use the overlay extention
d.findHostableTiles(allowDeadends=True, allowCorridor=True, allowCaves=True)

# add enterance/exit overlay tiles - if last 3 are false you've removed all tiles from possible use
d.placeEnteranceExitOverlay(allowOthers=False, allowDeadends=False, allowCorridor=False, allowRooms=True, allowCaves=True)

# load some overlay items
# placeRandomOverlays(overlay, count, hostType = None, setUnhostable = False)
d.placeRandomOverlays(dungeonGenerator.OVERLAY_ARTIFACT, 6, dungeonGenerator.FLOOR, True)
d.placeRandomOverlays(dungeonGenerator.OVERLAY_TREASURE, 10)
d.placeRandomOverlays(dungeonGenerator.OVERLAY_TREASURE, 15, dungeonGenerator.FLOOR)
d.placeRandomOverlays(dungeonGenerator.OVERLAY_TRAP, 6)
d.placeRandomOverlays(dungeonGenerator.OVERLAY_FOE, 6)


# Generate image from generated data - Using PIL Graphic Generator (PGG)
# ------------------------------------------------------------------------------------------
print("Map defined; building graphic")
PGG = graphicEngine_PIL.GraphicGenerator(d, "tilesets{0}structure.png".format(os.sep),\
                                         "tilesets{0}overlays.png".format(os.sep), tileSize=32)
PGG.saveImage("screenshot.jpg")

# we check now how long the code took to run.
print("Map Generated in %s" % (str(secondsToText(time() - start_time, 'EN'))))
