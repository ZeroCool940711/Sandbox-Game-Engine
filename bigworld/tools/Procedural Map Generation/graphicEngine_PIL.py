##################################################################
#                                                                #
# PIL Graphic Generator (PGG) for Procedural Dungeon Generator   #
# By GK101                                                       #
#                                                                #
# Feel free to use this as you wish, but please keep this header #
#                                                                #
##################################################################
import sys, os
import dungeonGenerator
from PIL import Image, ImageDraw, ImageFont

class GraphicGenerator():
    def __init__(self, dungeonObject, structTiles, overlayTiles, tileSize=32):
        self.d         = dungeonObject
        self.height    = dungeonObject.height
        self.width     = dungeonObject.width
        self.tileSize  = tileSize
        self.textColor = (128,255,0)
        self.font      = ImageFont.truetype("swissek.ttf",12)        

        self.imgStruct = Image.open(structTiles)
        self.imgOverly = Image.open(overlayTiles)

        self.keyTitle = ['Exit', 'Entrance', 'Foe', 'Trap', 'Artifact', 'Treasure']

        img_height = ((self.height+len(self.keyTitle))+2)*self.tileSize
        img_width  = (self.width+5)*self.tileSize
        self.image = Image.new('RGB', (img_width, img_height))

        self.overlayKey()
        
    def overlayText(self, x, y, background, text):
        bg  = ImageDraw.Draw(background)
        bg.text((x*self.tileSize, y*self.tileSize), text, font=self.font, fill=self.textColor)
        return
    
    def overlayGraphic(self, x, y, background, foreground):
        im=Image.open(foreground)
        background.paste(im, (x*self.tileSize, y*self.tileSize), im)
        return

    def overlayKey(self):
        for x in range(len(self.keyTitle)):
            y = (self.height+x+1)*self.tileSize
            o = self.getTile(self.imgOverly,x,0)
            
            self.image.paste(o, (0, y), o)

            pad = (self.tileSize/3)
            bg  = ImageDraw.Draw(self.image)
            bg.text((self.tileSize+(pad/2), y+pad), self.keyTitle[x],\
                    font=self.font, fill=self.textColor)
        return
    
    def displayDeadends(self):
        im = self.getTile(self.imgStruct,dungeonGenerator.DEADEND,0)
        for de in self.d.deadends:
            self.image.paste(im, (de[0] * self.tileSize, de[1] * self.tileSize), im)

    def getTile(self, img, tileX, tileY):
        posX = (self.tileSize * tileX)
        posY = (self.tileSize * tileY)
        box = (posX, posY, posX + self.tileSize, posY + self.tileSize)
        return img.crop(box)
    
    def saveImage(self, path):
        floorGraphicTiles = [dungeonGenerator.FLOOR, dungeonGenerator.CORRIDOR, dungeonGenerator.CAVE]
        for x, y, tile in self.d:
            im = None
            if tile != dungeonGenerator.DOOR and tile <= 9:
                im = self.getTile(self.imgStruct,tile,0)
            elif tile == dungeonGenerator.DOOR:
                im = self.getTile(self.imgStruct,tile,0)
                if self.d.grid[x+1][y] != dungeonGenerator.WALL:
                    im = im.rotate(90)

            if im != None:
                self.image.paste(im, (x*self.tileSize, y*self.tileSize), im)
        
        overlayId = 0
        cellBreakdown = 'Cell Id break down'
        for key, value in self.d.overlays.items():
            x = value[0]
            y = value[1]    
            i = ''
            l = value[2].items()
            o = len(l)
            
            # place a the graphics
            for t,v in l:
                im = None
                if t <= 5:
                    if o <= 1:
                        im = self.getTile(self.imgOverly,t,0)
                    i += '  {0}-{1}\n'.format(v,self.keyTitle[t])        
                else:
                    i += '  {0}-{1}\n'.format(v,'UNKNOWN')
                    
                if im != None:
                    self.image.paste(im, (x*self.tileSize, y*self.tileSize), im)

            # clear graphic if multiple items
            if o > 1:
                overlayId += 1
                self.overlayText(x,y,self.image, "{0}".format(overlayId))
                cellBreakdown += "\n({0})\n{1}".format(overlayId,i)

        self.overlayText(self.width,0,self.image, cellBreakdown)
        self.displayDeadends()
        self.image.save(path)
        self.imgStruct.close()
        self.imgOverly.close()
        return
