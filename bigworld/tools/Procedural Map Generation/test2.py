import sys

# pip install Pillow
from PIL import Image, ImageDraw, ImageFont
import dungeonGenerator, random, datetime, os
from time import time
from datetime import timedelta

# define map size (x,y) max tiles to use in direction
#levelSize = [50,50]

# seeding the random generator before any calls, ensure all dungeons are the same, so you can see
# the difference between each script modification or rendering approach.
# Set fixedSeed to false for random dungeons
fixedSeed = True

# define tile size in pixel count
tileSize = 32       

if fixedSeed:
    print ("Using fixed seed for testing, set fixedSeed to False to use random seed.")
    random.seed(1)

if not fixedSeed:
    # creating the seed based on the time of a computer or server could help if for example
    # somenoe makes a report but didnt took the time to look at the seed number, this way
    # we could track down the seed based on the time the report or issue was made 
    # or the time the person found the bug.
    tim = datetime.datetime.now()
    randseed = tim.hour*10000+tim.minute*100+tim.second
    random.seed(randseed)

# we use the time module to check how long it takes for the script to run
# this way we can know if the script is faster or not when we add something.
start_time = time()

# txt or image function
def overlayText(x,y,background,text):
    bg  = ImageDraw.Draw(background)
    ftp = ImageFont.load_default()
    fnt = ImageFont.truetype("swissek.ttf",12)
    bg.text((x*tileSize, y*tileSize), text, font=fnt, fill=(128,255,0))
    return

def overlayGraphic(x,y,background,foreground):
    im=Image.open(foreground).convert("RGBA")
    background.paste(im, (x*tileSize, y*tileSize), im)

def open_area_with_rooms(height=50, width=50):
    """This is an example of how to create an open area with rooms and each room connected by a road,
    this combines the the placeRandomRooms() with the generateCaves() function. By using
    first placeRandomRooms() and then generateCaves() we make sure that the rooms are inside the area
    that is created by generateCaves(), then we generate the corridor connecting the rooms and replace
    the corridor tiles by road tiles."""
    
    # create class instance; ALWAYS required
    d = dungeonGenerator.dungeonGenerator(height, width)
    
    # toggle to change generation type.
    # if useCaveFeature and useDungeonRooms are True it will create open areas
    # with hidden rooms, good for placing hidden chests, artifacts or puzzles.
    useCaveFeature = False
    useDungeonRooms = True
    
    if useCaveFeature and useDungeonRooms:
        
        d.generateCaves(35, 4)
        
        # generate rooms and corridors
        d.placeRandomRooms(5, 9, 1, 1, 500)
        #d.generateCaves(35, 4)
        
        x, y = d.findEmptySpace(7)
        while x:
            d.generateCorridors('l', x, y)
            x, y = d.findEmptySpace(7)
        # join it all together
        #d.connectAllRooms(0)
        d.mergeUnconnectedAreas()
        d.pruneDeadends(70)
    
    else:
        # generate map structure as caves
        if useCaveFeature:
            # generate the basic caves
            d.generateCaves(40, 4)
            
            d.mergeUnconnectedAreas()
        
        # generate map with rooms/corridors
        if not useCaveFeature:
            # generate room areas
            d.placeRandomRooms(5, 9, 1, 1, 500)
            # build corridors in empty spaces
            d.generateCorridors('r')
            # connect paths so rooms are accessable
            d.connectAllRooms(0)
            d.mergeUnconnectedAreas()
            # purge deadends; not required but makes things cleaner looking
            d.pruneDeadends(10) 
    
    
    # always call place walls and water after basic structure is made
    d.placeWalls()
    d.placeWater()
    
    
    # Overlay Extention Example
    # ---------------------------------
    print("Map structure made; generating defined overlays")
    
    # make call to find/define hostable tiles for use later on; this must be called to use the overlay extention
    d.findHostableTiles(allowDeadends=True, allowCorridor=True, allowCaves=True)
    
    # add enterance/exit overlay tiles - if last 3 are false you've removed all tiles from possible use
    d.placeEnteranceExitOverlay(allowOthers=False, allowDeadends=False, allowCorridor=False, allowRooms=True, allowCaves=True)
    
    # load some overlay items
    # placeRandomOverlays(overlay, count, hostType = None, setUnhostable = False)
    #d.placeRandomOverlays(dungeonGenerator.OVERLAY_ARTIFACT, 6, dungeonGenerator.FLOOR, True)
    #d.placeRandomOverlays(dungeonGenerator.OVERLAY_TREASURE, 10)
    d.placeRandomOverlays(dungeonGenerator.OVERLAY_TREASURE, 6, dungeonGenerator.FLOOR)
    d.placeRandomOverlays(dungeonGenerator.OVERLAY_TRAP, 6, dungeonGenerator.CORRIDOR)
    d.placeRandomOverlays(dungeonGenerator.OVERLAY_FOE, 6)
    d.placeRandomOverlays(dungeonGenerator.OVERLAY_ARMOUR_CHEST, 2)
    d.placeRandomOverlays(dungeonGenerator.OVERLAY_WEAPON_CHEST, 2)
    
    # Generate image from generated data
    # ------------------------------------------------------------------------------------------
    print("Map defined; building graphic")
    # list floor graphic tiles
    floorGraphicTiles = [dungeonGenerator.FLOOR, dungeonGenerator.CORRIDOR, dungeonGenerator.CAVE]
    
    # define possible tile overlay types for key size; image height extention
    overlayTypes = 5
    
    # create blank graphic to hold images
    #                          Width extention            Height Extention
    #                          overlaping overlays text , Symbol key area
    new_im = Image.new('RGB', ((width+5)*tileSize, (height+overlayTypes+2)*tileSize))
    
    
    # Load Map structure graphic
    for x, y, tile in d:
        im = None
        if tile == dungeonGenerator.EMPTY:
            im=Image.open('data{0}tilesets{0}tile_EMPTY.png'.format(os.sep))        
        elif tile in floorGraphicTiles:
            im=Image.open('data{0}tilesets{0}tile_FLOOR.png'.format(os.sep))
        elif tile == dungeonGenerator.WATER:
            im=Image.open('data{0}tilesets{0}dungeon{0}water{0}dngn_shallow_water.png'.format(os.sep))        
        elif tile == dungeonGenerator.WALL:
            im=Image.open('data{0}tilesets{0}tile_WALL.png'.format(os.sep))
        elif tile == dungeonGenerator.DOOR:
            # rotate the door tile accordingly
            # no need to check bounds since a door tile will never be against the edge
            # we want the door to use transparencies so its a little more complex
            if d.grid[x+1][y] == dungeonGenerator.WALL and d.grid[x-1][y] == dungeonGenerator.WALL:
                im=Image.open('data{0}tilesets{0}tile_DOOR-F.png'.format(os.sep))
            else:
                im=Image.open('data{0}tilesets{0}tile_DOOR-S.png'.format(os.sep))
                
            im2=Image.open('data{0}tilesets{0}tile_FLOOR.png'.format(os.sep))
            new_im.paste(im2, (x*tileSize, y*tileSize))
            new_im.paste(im, (x*tileSize, y*tileSize), im)
            im = None
    
        if im != None:
            new_im.paste(im, (x*tileSize, y*tileSize))
    
    #Here we create a list of files from a folder to use the images for a random overlay
    armour_item_list = os.listdir('data{0}tilesets{0}item{0}armour'.format(os.sep))
    weapon_item_list = os.listdir('data{0}tilesets{0}item{0}weapon'.format(os.sep))
    
    #print('data{0}tilesets{0}item{0}armour{0}'.format(os.sep) + str(random.choice(armour_item_list)))
    
    # load overlay tiles
    overlayId = 0
    cellBreakdown = 'Cell Id break down'
    for key, value in d.overlays.items():
        x = value[0]
        y = value[1]    
        i = ''
        l = value[2].items()
        o = len(l)
        
        # place a the graphics
        for t,v in l:
            f = None
            if t == dungeonGenerator.OVERLAY_ARTIFACT:
                f = 'data{0}tilesets{0}tile_ARTIFACT.png'.format(os.sep)
                i += '  {0}-{1}\n'.format(v,'Artifacts')            
            elif t == dungeonGenerator.OVERLAY_TREASURE:
                f = 'data{0}tilesets{0}tile_TREASURE.png'.format(os.sep)
                i += '  {0}-{1}\n'.format(v,'Treasure')  
            elif t == dungeonGenerator.OVERLAY_ARMOUR_CHEST:
                f = 'data{0}tilesets{0}item{0}armour{0}'.format(os.sep) + str(random.choice(armour_item_list))
                i += '  {0}-{1}\n'.format(v,'Armour Chest')        
            elif t == dungeonGenerator.OVERLAY_WEAPON_CHEST:
                f = 'data{0}tilesets{0}item{0}weapon{0}'.format(os.sep) + str(random.choice(weapon_item_list))
                i += '  {0}-{1}\n'.format(v,'Weapon Chest')                
            elif t == dungeonGenerator.OVERLAY_TRAP:
                f = 'data{0}tilesets{0}tile_TRAP.png'.format(os.sep)
                i += '  {0}-{1}\n'.format(v,'Trap')            
            elif t == dungeonGenerator.OVERLAY_FOE:
                f = 'data{0}tilesets{0}tile_FOE.png'.format(os.sep)
                i += '  {0}-{1}\n'.format(v,'Foe')
            elif t == dungeonGenerator.OVERLAY_EXIT:
                i += '  {0}-{1}\n'.format(v,'Exit')
                f = 'data{0}tilesets{0}tile_EXIT.png'.format(os.sep)            
            elif t == dungeonGenerator.OVERLAY_ENTER:
                i += '  {0}-{1}\n'.format(v,'Entrance')
                f = 'data{0}tilesets{0}tile_ENTER.png'.format(os.sep)         
            else:
                i += '  {0}-{1}\n'.format(v,'UNKNOWN')
                
            if f != None:
                overlayGraphic(x,y,new_im,f)
    
    
    # load deadend graphics like very last or atleast after the enter{0}exit
    # IDK when these would get used ... but its a demo script
    for de in d.deadends:
        im=Image.open('data{0}tilesets{0}tile_DEADEND.png'.format(os.sep))
        new_im.paste(im, (de[0] * tileSize, de[1] * tileSize), im)
    
    # save image
    new_im.save("screenshot.jpg")       

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




## Load Map structure graphics
#for x, y, tile in d:
    #im = None
    #if tile == dungeonGenerator.EMPTY:
        #im=Image.open('data{0}tilesets{0}tile_EMPTY.png'.format(os.sep))        
    #elif tile in floorGraphicTiles:
        #im=Image.open('data{0}tilesets{0}tile_FLOOR.png'.format(os.sep))
    #elif tile == dungeonGenerator.WATER:
        #im=Image.open('data{0}tilesets{0}dungeon{0}water{0}dngn_shallow_water.png'.format(os.sep))        
    #elif tile == dungeonGenerator.WALL:
        #im=Image.open('data{0}tilesets{0}tile_WALL.png'.format(os.sep))
    #elif tile == dungeonGenerator.DOOR:
        ## rotate the door tile accordingly
        ## no need to check bounds since a door tile will never be against the edge
        ## we want the door to use transparencies so its a little more complex
        #if d.grid[x+1][y] == dungeonGenerator.WALL:
            #im=Image.open('data{0}tilesets{0}tile_DOOR-F.png'.format(os.sep))
        #else:
            #im=Image.open('data{0}tilesets{0}tile_DOOR-S.png'.format(os.sep))
            
        #im2=Image.open('data{0}tilesets{0}tile_FLOOR.png'.format(os.sep))
        #new_im.paste(im2, (x*tileSize, y*tileSize))
        #new_im.paste(im, (x*tileSize, y*tileSize), im)
        #im = None

    #if im != None:
        #new_im.paste(im, (x*tileSize, y*tileSize))

## Load Origin graphic - Not really needed ... but demo file. 
##im=Image.open('data{0}tilesets{0}tile_ORIGIN.png'.format(os.sep))
##new_im.paste(im, (0,0), im)

## Load Graphic key
##overlayGraphic(0,levelSize[0]+1,new_im,'data{0}tilesets{0}tile_ENTER.png'.format(os.sep))
##overlayText(1,levelSize[0]+1,new_im, 'Entrance')
##overlayGraphic(0,levelSize[0]+2,new_im,'data{0}tilesets{0}tile_EXIT.png'.format(os.sep))
##overlayText(1,levelSize[0]+2,new_im, 'Exit')
##overlayGraphic(0,levelSize[0]+3,new_im,'data{0}tilesets{0}tile_TREASURE.png'.format(os.sep))
##overlayText(1,levelSize[0]+3,new_im, 'Treasure')
##overlayGraphic(0,levelSize[0]+4,new_im,'data{0}tilesets{0}tile_TRAP.png'.format(os.sep))
##overlayText(1,levelSize[0]+4,new_im, 'Trap')
##overlayGraphic(0,levelSize[0]+5,new_im,'data{0}tilesets{0}tile_FOE.png'.format(os.sep))
##overlayText(1,levelSize[0]+5,new_im, 'Foe')
##overlayGraphic(0,levelSize[0]+6,new_im,'data{0}tilesets{0}tile_ARTIFACT.png'.format(os.sep))
##overlayText(1,levelSize[0]+6,new_im, 'Artifacts')


##Here we create a list of files from a folder to use the images for a random overlay
#armour_item_list = os.listdir('data{0}tilesets{0}item{0}armour'.format(os.sep))
#weapon_item_list = os.listdir('data{0}tilesets{0}item{0}weapon'.format(os.sep))

##print('data{0}tilesets{0}item{0}armour{0}'.format(os.sep) + str(random.choice(armour_item_list)))

## load overlay tiles
#overlayId = 0
#cellBreakdown = 'Cell Id break down'
#for key, value in d.overlays.items():
    #x = value[0]
    #y = value[1]    
    #i = ''
    #l = value[2].items()
    #o = len(l)
    
    ## place a the graphics
    #for t,v in l:
        #f = None
        #if t == dungeonGenerator.OVERLAY_ARTIFACT:
            #f = 'data{0}tilesets{0}tile_ARTIFACT.png'.format(os.sep)
            #i += '  {0}-{1}\n'.format(v,'Artifacts')            
        #elif t == dungeonGenerator.OVERLAY_TREASURE:
            #f = 'data{0}tilesets{0}tile_TREASURE.png'.format(os.sep)
            #i += '  {0}-{1}\n'.format(v,'Treasure')  
        #elif t == dungeonGenerator.OVERLAY_ARMOUR_CHEST:
            #f = 'data{0}tilesets{0}item{0}armour{0}'.format(os.sep) + str(random.choice(armour_item_list))
            #i += '  {0}-{1}\n'.format(v,'Armour Chest')        
        #elif t == dungeonGenerator.OVERLAY_WEAPON_CHEST:
            #f = 'data{0}tilesets{0}item{0}weapon{0}'.format(os.sep) + str(random.choice(weapon_item_list))
            #i += '  {0}-{1}\n'.format(v,'Weapon Chest')                
        #elif t == dungeonGenerator.OVERLAY_TRAP:
            #f = 'data{0}tilesets{0}tile_TRAP.png'.format(os.sep)
            #i += '  {0}-{1}\n'.format(v,'Trap')            
        #elif t == dungeonGenerator.OVERLAY_FOE:
            #f = 'data{0}tilesets{0}tile_FOE.png'.format(os.sep)
            #i += '  {0}-{1}\n'.format(v,'Foe')
        #elif t == dungeonGenerator.OVERLAY_EXIT:
            #i += '  {0}-{1}\n'.format(v,'Exit')
            #f = 'data{0}tilesets{0}tile_EXIT.png'.format(os.sep)            
        #elif t == dungeonGenerator.OVERLAY_ENTER:
            #i += '  {0}-{1}\n'.format(v,'Entrance')
            #f = 'data{0}tilesets{0}tile_ENTER.png'.format(os.sep)         
        #else:
            #i += '  {0}-{1}\n'.format(v,'UNKNOWN')
            
        #if f != None:
            #overlayGraphic(x,y,new_im,f)

    ## clear graphic if multiple items
    ##if o > 1:
        ##overlayId += 1
        ### cover what might have been on the cell
        ##overlayGraphic(x,y,new_im, 'data{0}tilesets{0}tile_FLOOR.png'.format(os.sep))
        ### write ID onto cell
        ##overlayText(x,y,new_im, "{0}".format(overlayId))
        ### update text chunk
        ##cellBreakdown += "\n({0})\n{1}".format(overlayId,i)           
    
## Load key data for crowded cells
##overlayText(levelSize[1],0,new_im, cellBreakdown)

## load deadend graphics like very last or atleast after the enter{0}exit
## IDK when these would get used ... but its a demo script
#for de in d.deadends:
    #im=Image.open('data{0}tilesets{0}tile_DEADEND.png'.format(os.sep))
    #new_im.paste(im, (de[0] * tileSize, de[1] * tileSize), im)



open_area_with_rooms(50, 50)

# we check now how long the code took to run.
print("Map Generated in %s" % (str(secondsToText(time() - start_time, 'EN'))))