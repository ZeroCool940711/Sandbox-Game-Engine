# This example shows setting a specific tile
# This is useful if you always want to gaurantee a tile is present, for connecting dungeons or whatever

#import bge
import dungeonGenerator
import random, sys, datetime, os

from yattag import Doc, indent
from time import time
from datetime import timedelta

import threading

# seeding the random generator before any calls, ensure all dungeons are the same, so you can see
# the difference between each rendering approach
# comment this out for random dungeons
#print ("Using fixed seed for testing, comment line 16 to use random seed.")
#random.seed(1)

# generating the seed this way creates a bigger seed number.
#seed = random.randrange(sys.maxsize)
#rng = random.Random(seed)
#print("The Seed was:", seed)

# creating the seed based on the time of a computer or server could help if for example
# a user make a report but didnt new the seed, this way we could track down the seed
# based on the time the report was made or the time the user found the bug.
tim = datetime.datetime.now()
randseed = tim.hour*10000+tim.minute*100+tim.second
#random.seed(randseed)

print ("Creating Random Terrain.")
print("The seed is:", randseed)

def simpleTile(height=30, width=30):
	d = dungeonGenerator.dungeonGenerator(height, width)
	
	# set a specific tile in the grid
	# doing this before generating the dungeon will mean that the dungeon will be built around the tile
	# you could do it after the generation as well for different results
	d.grid[15][15] = dungeonGenerator.FLOOR
	
	d.placeRandomRooms(5, 9, 2, 3, 1000)
	d.generateCorridors('l')
	d.connectAllRooms(30)
	d.pruneDeadends(30)
	
	# since nothing else will be placed around the set tile it wont be connected to anything
	# so we need to find the disconnected areas and join them
	unconnected = d.findUnconnectedAreas()
	d.joinUnconnectedAreas(unconnected)
	
	# render out the grid
	# notice that up until this point there's been no Blender specific python
	#own = bge.logic.getCurrentController().owner
	#scene = bge.logic.getCurrentScene()
	tileSpacing = 7
	
	doc, tag, text = Doc().tagtext()	
	
	with tag('root'):
		with tag('terrain'):
			with tag('resource'):
				text('00000000o.cdata/terrain2')
			with tag('editorOnly'):
				with tag('hidden'):
					text('true')
				with tag('frozen'):
					text('false')
			with tag('metaData'):
				text('')
			
		for x, y, tile in d:
			if tile:
				# lets add our set tile in a different colour
				# we can do this since we know it's position in the grid (15,15)
				#if x == 15 and y == 15:
				#ob = scene.addObject('Floor2', own, 0)
				#else:
				#ob = scene.addObject('Floor', own, 0)
				#ob.worldPosition.x += x * tileSpacing
				#ob.worldPosition.y += y * tileSpacing  
				#ob.worldPosition.z -= 1
				x += x * tileSpacing 
				y += y * tileSpacing
				z = 0
				
				arrx = x
				arry = z
				arrz = y	
				
				with tag('model'):
					with tag('editorOnly'):
						with tag('hidden'):
							text('false')
							with tag('frozen'):
								text('false')		
					with tag('metaData'):
						with tag('created_by'):
							text('Alejandro')
						with tag('created_on'):
							text(unicode(time()).partition('.')[0])
						with tag('modified_by'):
							text('Alejandro')
						with tag('modified_on'):
							text(unicode(time()).partition('.')[0])
					
					with tag('resource'):
						text('characters/others/stone_floor_1x1.model')
					with tag('transform'):
						with tag('row0'):
							text('2 0 0')
						with tag('row1'):
							text('0 2 0')
						with tag('row2'):
							text('0 0 2')
						with tag('row3'):
							text('{0} {1} {2}'.format(arrx, arry, arrz))
					with tag('castsShadow'):
						text('true')
					with tag('reflectionVisible'):
						text('false')
				with tag('worldNavmesh'):
					with tag('resource'):
						text('00000000o.cdata/worldNavmesh')
					
	result = indent(
	        doc.getvalue(),
	        indentation = ' '*4,
	        newline = '\r\n'
	)

	print(result)	
		

def wallsAttachedToFloorTilesWithDoors(height=30,width=30):
	
	d = dungeonGenerator.dungeonGenerator(height, width)
	
	#d.placeRandomRooms(3, 9, 2, 3, 1000)
	placeRandomRooms = threading.Thread(target=d.placeRandomRooms, kwargs={'minRoomSize':3, 'maxRoomSize':9, 'roomStep':2, 'margin':3, 'attempts':1000}).start()
	
	#d.generateCaves(40, 4)
	d.generateCorridors('r')
	
	# generate door tiles
	#d.connectAllRooms(30)
	connectAllRooms = threading.Thread(target=d.connectAllRooms, kwargs={'extraDoorChance':30}).start()
	d.pruneDeadends(30)
	#pruneDeadends = threading.Thread(target=d.pruneDeadends, kwargs={'amount':30}).start()
	
	tileSpacing = 7
	
	doc, tag, text = Doc().tagtext()
	
	model = 'characters/others/dungeon/smooth_floor_2x2.model'
	row0 = '0.800 0 0'
	row1 = '0 0.800 0'
	row2 = '0 0 0.800'
	row3 = '0 0 0'
	created_on = ''
	modified_on = ''
	
	
	#row0 = '-1.999998 0.000691 0.000000'
	#row1 = '0.000691 1.999999 -0.000000'
	#row2 = '-0.000000 0.000000 -1.999999'
	
	with tag('root'):
		with tag('terrain'):
			with tag('resource'):
				text('00000000o.cdata/terrain2')
			with tag('editorOnly'):
				with tag('hidden'):
					text('false')
				with tag('frozen'):
					text('false')
			with tag('metaData'):
				text('')
				
		for x, y, tile in d:
			# don't need to check what type the tile is just that a tile exists
			if tile:
		
				# filter the tiles depending on how many other ones they're touching
				touching = 0
				tileType = 0
				for nx, ny in d.findNeighboursDirect(x, y):
					if d.grid[nx][ny]:
						touching += 1
				# if touching 1 tile we've got a deadend
				if touching == 1:
					tileType = 'deadend'
					# ob = scene.addObject('FloorDeadend', own, 0)
					model = 'characters/others/dungeon/smooth_dead_end_2x2.model'
					row0 = '-0.800 0.000000 0.000000'
					row1 = '0.000000 0.800 0.000000'
					row2 = '0.000000 0.000000 -0.800'
					# bottom facing
					if d.grid[x][y+1]:
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = row0
						row1 = row1
						row2 = row2
						
					# right facing
					elif d.grid[x+1][y]:
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '-0.800 0.000000 0.000000'
						
					# left facing
					elif d.grid[x-1][y]:
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.000000 0.000691 -0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.800 0.000000 0.000000'
						
		
				# if they're only touching 2 other tiles we've got a corridor section
				# or a door. So this time round we're gonna check the tile type and render out
				# a door or floor section
				elif touching == 2:
					# up-down section
					#print d.grid[x][0]
					if d.grid[x][y+1] and d.grid[x][y-1]:
						# if we've got a door tile, add that instead of a corridor
						if tile == dungeonGenerator.DOOR:
							# ob = scene.addObject('DoorStraight', own, 0)
							model = 'characters/others/dungeon/smooth_corridor_2x2.model'
							tileType = 'DoorStraight'
						else:
							# ob = scene.addObject('FloorStraight', own, 0)
							model = 'characters/others/dungeon/smooth_corridor_2x2.model'
							tileType = 'FloorStraight'
						row0 = '0.000000 0.000691 -0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.800 0.000000 0.000000'
					# left-right section
					elif d.grid[x+1][y] and d.grid[x-1][y]:
						if tile == dungeonGenerator.DOOR:
							# ob = scene.addObject('DoorStraight', own, 0)
							model = 'characters/others/dungeon/smooth_corridor_2x2.model'
							tileType = 'DoorStraight'
						else:
							# ob = scene.addObject('FloorStraight', own, 0)
							model = 'characters/others/dungeon/smooth_corridor_2x2.model'
							tileType = 'FloorStraight'
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.800 0.000000 0.000000'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.000000 0.000000 0.800'
		
					# otherwise, we've got a corner, so lets figure out which type
					# top-left
					elif d.grid[x+1][y] and d.grid[x][y-1]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'characters/others/dungeon/smooth_edge_corner_2x2.model'
						tileType = 'FloorCorner'
						row0 = '0.000000 0.000000 0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '-0.800 0.000000 0.000000'						
					# top-right
					elif d.grid[x-1][y] and d.grid[x][y-1]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'characters/others/dungeon/smooth_edge_corner_2x2.model'
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.800 0.000000 0.000000'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.000000 0.000000 0.800'
						
						tileType = 'FloorCorner'
					# bottom-left
					elif d.grid[x][y+1] and d.grid[x+1][y]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'characters/others/dungeon/smooth_edge_corner_2x2.model'
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '-0.800 0.000000 0.000000'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.000000 0.000000 -0.800'
						
						tileType = 'FloorCorner'
					# bottom-right
					else:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'characters/others/dungeon/smooth_edge_corner_2x2.model'
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = '0.000000 0.000691 -0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.800 0.000000 0.000000'
						
						tileType = 'FloorCorner'
		
				# if they're touching 3 others we've got a room wall, or a corridor tile opposite a door tile
				elif touching == 3:
					tileType = 'linearWall'
					# ob = scene.addObject('FloorSingleWall', own, 0)
					model = 'characters/others/dungeon/smooth_edge_wall_2x2.model'
					row0 = '0.800 0.000000 0.000000'
					row1 = '0.000000 0.800 0.000000'
					row2 = '0.000000 0.000000 0.800'					
					# bottom facing
					if not d.grid[x][y-1]:
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = '-0.800 0.000000 0.000000'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.000000 0.000000 -0.800'
						
					# right facing
					elif not d.grid[x+1][y]:
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.000000 0.000691 -0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '0.800 0.000000 0.000000'
					# left facing
					elif not d.grid[x-1][y]:
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 0.800'
						row1 = '0.000000 0.800 0.000000'
						row2 = '-0.800 0.000000 0.000000'
		
				# we've got a tile that's in the centre of a room
				else:
					#ob = scene.addObject('Floor', own, 0)
					#model = 'characters/others/smooth_floor_2x2.model'
					model = 'characters/others/dungeon/smooth_floor_2x2.model'
					tileType = 'roomCenter'
					
				x += x * tileSpacing 
				y += y * tileSpacing
				z = 0
				
				arrx = x
				arry = z
				arrz = y
				
				row3 = '{0} {1} {2}'.format(arrx, arry, arrz)
				
				with tag('model'):
					with tag('editorOnly'):
						with tag('hidden'):
							text('false')
							with tag('frozen'):
								text('false')		
					with tag('metaData'):
						with tag('created_by'):
							text('Alejandro')
						with tag('created_on'):
							text(unicode(time()).partition('.')[0])
						with tag('modified_by'):
							text('Alejandro')
						with tag('modified_on'):
							text(unicode(time()).partition('.')[0])
						with tag('description'):
							text(tileType)
					
					with tag('resource'):
						if tileType == 0:
							text('characters/others/dungeon/smooth_floor_2x2.model')
						else:
							text(model)
					with tag('transform'):
						with tag('row0'):
							text(row0)
						with tag('row1'):
							text(row1)
						with tag('row2'):
							text(row2)
						with tag('row3'):
							text(row3)
					with tag('castsShadow'):
						text('true')
					with tag('reflectionVisible'):
						text('true')
		with tag('worldNavmesh'):
			with tag('resource'):
				text('00000000o.cdata/worldNavmesh')				
				
		
	result = indent(
	        doc.getvalue(),
	        indentation = ' '*4,
	        newline = '\r\n'
	)
	
	#print(result)
	
	print ("Done")
	
	return result


def open_area_generation_not_working(height=30,width=30):
	
	d = dungeonGenerator.dungeonGenerator(height, width)
	#d.placeRandomRooms(5, 9, 2, 3, 1000)
	#d.generateCorridors('m')
	d.generateCaves(p=60,smoothing=4)
	# generate door tiles
	#d.connectAllRooms(30)
	d.pruneDeadends(30)

	tileSpacing = 7

	# since nothing else will be placed around the set tile it wont be connected to anything
	# so we need to find the disconnected areas and join them
	unconnected = d.findUnconnectedAreas()
	d.joinUnconnectedAreas(unconnected)
	
	doc, tag, text = Doc().tagtext()
	
	model = 'sets/temperate/props/flagstone_slab.model'
	row0 = '1.999999 0 0'
	row1 = '0 1.999999 0'
	row2 = '0 0 1.999999'
	row3 = '0 0 0'
	created_on = ''
	modified_on = ''
	
	
	#row0 = '-1.999998 0.000691 0.000000'
	#row1 = '0.000691 1.999999 -0.000000'
	#row2 = '-0.000000 0.000000 -1.999999'
	
	with tag('root'):
		with tag('terrain'):
			with tag('resource'):
				text('00000000o.cdata/terrain2')
			with tag('editorOnly'):
				with tag('hidden'):
					text('false')
				with tag('frozen'):
					text('false')
			with tag('metaData'):
				text('')
				
		for x, y, tile in d:
			# don't need to check what type the tile is just that a tile exists
			if tile:
		
				# filter the tiles depending on how many other ones they're touching
				touching = 0
				tileType = 0
				for nx, ny in d.findNeighboursDirect(x, y):
					if d.grid[nx][ny]:
						touching += 1
				# if touching 1 tile we've got a deadend
				if touching == 1:
					# ob = scene.addObject('FloorDeadend', own, 0)
					model = 'sets/temperate/props/flagstone_slab.model'
					# bottom facing
					if d.grid[x][y+1]:
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = row0
						row1 = row1
						row2 = row2
						
					# right facing
					elif d.grid[x+1][y]:
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '-1.999998 0.000000 0.000000'
						
					# left facing
					elif d.grid[x-1][y]:
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.000000 0.000691 -1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '1.999999 0.000000 0.000000'
						
		
				# if they're only touching 2 other tiles we've got a corridor section
				# or a door. So this time round we're gonna check the tile type and render out
				# a door or floor section
				elif touching == 2:
					# up-down section
					print (d.grid[x][0])
					if d.grid[x][y+1] and d.grid[x][y-1]:
						# if we've got a door tile, add that instead of a corridor
						if tile == dungeonGenerator.DOOR:
							# ob = scene.addObject('DoorStraight', own, 0)
							model = 'sets/temperate/props/flagstone_slab.model'
						else:
							# ob = scene.addObject('FloorStraight', own, 0)
							model = 'sets/temperate/props/flagstone_slab.model'
					# left-right section
					elif d.grid[x+1][y] and d.grid[x-1][y]:
						if tile == dungeonGenerator.DOOR:
							# ob = scene.addObject('DoorStraight', own, 0)
							model = 'sets/temperate/props/flagstone_slab.model'
						else:
							# ob = scene.addObject('FloorStraight', own, 0)
							model = 'sets/temperate/props/flagstone_slab.model'
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '-1.999998 0.000000 0.000000'
		
					# otherwise, we've got a corner, so lets figure out which type
					# top-left
					elif d.grid[x+1][y] and d.grid[x][y-1]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'sets/temperate/props/flagstone_slab.model'
					# top-right
					elif d.grid[x-1][y] and d.grid[x][y-1]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'sets/temperate/props/flagstone_slab.model'
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.000000 0.000691 -1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '1.999999 0.000000 0.000000'
					# bottom-left
					elif d.grid[x][y+1] and d.grid[x+1][y]:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'sets/temperate/props/flagstone_slab.model'
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '-1.999998 0.000000 0.000000'
					# bottom-right
					else:
						# ob = scene.addObject('FloorCorner', own, 0)
						model = 'sets/temperate/props/flagstone_slab.model'
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = '0.000000 0.000691 -1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '1.999999 0.000000 0.000000'
		
				# if they're touching 3 others we've got a room wall, or a corridor tile opposite a door tile
				elif touching == 3:
					# ob = scene.addObject('FloorSingleWall', own, 0)
					model = 'sets/temperate/props/flagstone_slab.model'
					# bottom facing
					if not d.grid[x][y-1]:
						#ob.worldOrientation = [0, 0, 3.1415]
						row0 = row0
						row1 = row1
						row2 = row2
						
					# right facing
					elif not d.grid[x+1][y]:
						#ob.worldOrientation = [0, 0, -1.5708]
						row0 = '0.000000 0.000691 -1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '1.999999 0.000000 0.000000'
					# left facing
					elif not d.grid[x-1][y]:
						#ob.worldOrientation = [0, 0, 1.5708]
						row0 = '0.000000 0.000000 1.999997'
						row1 = '0.000000 1.999999 0.000000'
						row2 = '-1.999998 0.000000 0.000000'
		
				# we've got a tile that's in the centre of a room
				else:
					#ob = scene.addObject('Floor', own, 0)
					model = 'sets/temperate/props/flagstone_slab.model'
					
				x += x * tileSpacing 
				y += y * tileSpacing
				z = 0
				
				arrx = x
				arry = z
				arrz = y
				
				row3 = '{0} {1} {2}'.format(arrx, arry, arrz)
				
				with tag('model'):
					with tag('editorOnly'):
						with tag('hidden'):
							text('false')
							with tag('frozen'):
								text('false')		
					with tag('metaData'):
						with tag('created_by'):
							text('Alejandro')
						with tag('created_on'):
							text(unicode(time()).partition('.')[0])
						with tag('modified_by'):
							text('Alejandro')
						with tag('modified_on'):
							text(unicode(time()).partition('.')[0])
						with tag('description'):
							text(tileType)
					
					with tag('resource'):
						text(model)
					with tag('transform'):
						with tag('row0'):
							text(row0)
						with tag('row1'):
							text(row1)
						with tag('row2'):
							text(row2)
						with tag('row3'):
							text(row3)
					with tag('castsShadow'):
						text('true')
					with tag('reflectionVisible'):
						text('true')
		with tag('worldNavmesh'):
			with tag('resource'):
				text('00000000o.cdata/worldNavmesh')				
				
		
	result = indent(
	        doc.getvalue(),
	        indentation = ' '*4,
	        newline = '\r\n'
	)
	
	print(result)	


def open_area_generation(height=30, width=30):
	d = dungeonGenerator.dungeonGenerator(height, width)
	#d.placeRandomRooms(5, 9, 2, 3, 1000)
	#d.generateCorridors('m')
	d.generateCaves(p=45,smoothing=4)
	# generate door tiles
	#d.connectAllRooms(30)
	#d.pruneDeadends(30)

	tileSpacing = 7

	# since nothing else will be placed around the set tile it wont be connected to anything
	# so we need to find the disconnected areas and join them
	unconnected = d.findUnconnectedAreas()
	d.joinUnconnectedAreas(unconnected)

	# render out the grid
	# notice that up until this point there's been no Blender specific python
	#own = bge.logic.getCurrentController().owner
	#scene = bge.logic.getCurrentScene()
	#tileSpacing = 7

	doc, tag, text = Doc().tagtext()
	
	model = 'characters/others/dungeon/smooth_floor_2x2.model'
	row0 = '0.793396 0 0'
	row1 = '0 0.793396 0'
	row2 = '0 0 0.793396'
	row3 = '0 0 0'
	created_on = ''
	modified_on = ''	

	with tag('root'):
		with tag('terrain'):
			with tag('resource'):
				text('00000000o.cdata/terrain2')
			with tag('editorOnly'):
				with tag('hidden'):
					text('true')
				with tag('frozen'):
					text('false')
			with tag('metaData'):
				text('')

		for x, y, tile in d:
			if tile:
				# lets add our set tile in a different colour
				# we can do this since we know it's position in the grid (15,15)
				#if x == 15 and y == 15:
				#ob = scene.addObject('Floor2', own, 0)
				#else:
				#ob = scene.addObject('Floor', own, 0)
				#ob.worldPosition.x += x * tileSpacing
				#ob.worldPosition.y += y * tileSpacing  
				#ob.worldPosition.z -= 1

				# filter the tiles depending on how many other ones they're touching
				touching = 0
				tileType = 0
				for nx, ny in d.findNeighboursDirect(x, y):
					if d.grid[nx][ny]:
						touching += 1	

				if touching == 1:
					tileType = 'deadend'
				elif touching == 2:
					#if d.grid[x][y+1] and d.grid[x][y-1]:
					# if we've got a door tile, add that instead of a corridor
					if tile == dungeonGenerator.DOOR:
						# ob = scene.addObject('DoorStraight', own, 0)
						tileType = 'door'
					else:
						# ob = scene.addObject('FloorStraight', own, 0)
						tileType = 'cornerWall'
				elif touching == 3:
					tileType = 'linearWall'
				else:
					tileType = 'roomCenter'

				x += x * tileSpacing 
				y += y * tileSpacing
				z = 0

				arrx = x
				arry = z
				arrz = y	

				with tag('model'):
					with tag('editorOnly'):
						with tag('hidden'):
							text('false')
							with tag('frozen'):
								text('false')		
					with tag('metaData'):
						with tag('created_by'):
							text('Alejandro')
						with tag('created_on'):
							text(unicode(time()).partition('.')[0])
						with tag('modified_by'):
							text('Alejandro')
						with tag('modified_on'):
							text(unicode(time()).partition('.')[0])
						with tag('description'):
							text(tileType)

					with tag('resource'):
						text('characters/others/dungeon/smooth_floor_2x2.model')
					with tag('transform'):
						with tag('row0'):
							text(row0)
						with tag('row1'):
							text(row1)
						with tag('row2'):
							text(row2)
						with tag('row3'):
							text('{0} {1} {2}'.format(arrx, arry, arrz))
					with tag('castsShadow'):
						text('true')
					with tag('reflectionVisible'):
						text('true')
				with tag('worldNavmesh'):
					with tag('resource'):
						text('00000000o.cdata/worldNavmesh')

	result = indent(
	        doc.getvalue(),
	        indentation = ' '*4,
	        newline = '\r\n'
	)
	
	#print result
	print ("Done")
	return result


def area_with_wall_woors_and_open_areas(height=50, width=50):
	d = dungeonGenerator.dungeonGenerator(height, width)
	d.generateCaves(40, 4)
	# clear away small islands
	unconnected = d.findUnconnectedAreas()
	for area in unconnected:
		if len(area) < 35:
			for x, y in area:
				d.grid[x][y] = dungeonGenerator.EMPTY
	# generate rooms and corridors
	d.placeRandomRooms(5, 9, 1, 1, 2000)
	x, y = d.findEmptySpace(3)
	while x:
		d.generateCorridors('l', x, y)
		x, y = d.findEmptySpace(3)
	# join it all together
	d.connectAllRooms(0)
	unconnected = d.findUnconnectedAreas()
	d.joinUnconnectedAreas(unconnected)
	d.pruneDeadends(70)	
	
	tileSpacing = 7

	doc, tag, text = Doc().tagtext()
	
	model = 'characters/others/dungeon/smooth_floor_2x2.model'
	row0 = '0.793396 0 0'
	row1 = '0 0.793396 0'
	row2 = '0 0 0.793396'
	row3 = '0 0 0'
	created_on = ''
	modified_on = ''	

	with tag('root'):
		with tag('terrain'):
			with tag('resource'):
				text('00000000o.cdata/terrain2')
			with tag('editorOnly'):
				with tag('hidden'):
					text('true')
				with tag('frozen'):
					text('false')
			with tag('metaData'):
				text('')

		for x, y, tile in d:
			if tile:
				# lets add our set tile in a different colour
				# we can do this since we know it's position in the grid (15,15)
				#if x == 15 and y == 15:
				#ob = scene.addObject('Floor2', own, 0)
				#else:
				#ob = scene.addObject('Floor', own, 0)
				#ob.worldPosition.x += x * tileSpacing
				#ob.worldPosition.y += y * tileSpacing  
				#ob.worldPosition.z -= 1

				# filter the tiles depending on how many other ones they're touching
				touching = 0
				tileType = 0
				for nx, ny in d.findNeighboursDirect(x, y):
					if d.grid[nx][ny]:
						touching += 1	

				if touching == 1:
					tileType = 'deadend'
				elif touching == 2:
					#if d.grid[x][y+1] and d.grid[x][y-1]:
					# if we've got a door tile, add that instead of a corridor
					if tile == dungeonGenerator.DOOR:
						# ob = scene.addObject('DoorStraight', own, 0)
						tileType = 'door'
					else:
						# ob = scene.addObject('FloorStraight', own, 0)
						tileType = 'cornerWall'
				elif touching == 3:
					tileType = 'linearWall'
				else:
					tileType = 'roomCenter'

				x += x * tileSpacing 
				y += y * tileSpacing
				z = 0

				arrx = x
				arry = z
				arrz = y	

				with tag('model'):
					with tag('editorOnly'):
						with tag('hidden'):
							text('false')
							with tag('frozen'):
								text('false')		
					with tag('metaData'):
						with tag('created_by'):
							text('Alejandro')
						with tag('created_on'):
							text(unicode(time()).partition('.')[0])
						with tag('modified_by'):
							text('Alejandro')
						with tag('modified_on'):
							text(unicode(time()).partition('.')[0])
						with tag('description'):
							text(tileType)

					with tag('resource'):
						text('characters/others/dungeon/smooth_floor_2x2.model')
					with tag('transform'):
						with tag('row0'):
							text(row0)
						with tag('row1'):
							text(row1)
						with tag('row2'):
							text(row2)
						with tag('row3'):
							text('{0} {1} {2}'.format(arrx, arry, arrz))
					with tag('castsShadow'):
						text('true')
					with tag('reflectionVisible'):
						text('true')
				with tag('worldNavmesh'):
					with tag('resource'):
						text('00000000o.cdata/worldNavmesh')

	result = indent(
            doc.getvalue(),
            indentation = ' '*4,
            newline = '\r\n'
    )
	
	#print result
	print ("Done")
	return result
	

"""To add a dye or material to a model from here use just add the tag <dye> and inside the name of the tint for the material, for example

    <dye>
	     <name>	Cap_04	</name>
		 <tint>	Cap	</tint>
	</dye>

"""

# we use the time module to check how long it takes for our script to run.
start_time = time()

"""This generator creaste a single map with a simple tileset, useful for testing."""
#simpleTile()

"""This generator creastes a open map with a single tileset type for now."""
#rng = open_area_generation(height=50, width=50)

"""This generator tries to combine all the other generators into one to create
maps with open areas, rooms, corridors, walls and doors"""
rng = area_with_wall_woors_and_open_areas(height=50, width=50)

"""This one creates a random terrain similar to a dungeon with walls, floor tiles,
corners and corredors. Use the height and width options to set the size of the map,
it has to be the same number for both options, if one is different than the other it
will throw and error, have to find a way to fix that."""
#rng = wallsAttachedToFloorTilesWithDoors(height=50, width=20)

# we check now how long the code took to run.
print("Terrain Generated in %s seconds" % (str(timedelta(seconds=time() - start_time))))

os.path.dirname(os.path.abspath(__file__))

with open(str(os.path.dirname(os.path.abspath(__file__)))+'\\res\\spaces\\ulgrim\\00000000o.chunk', 'wb') as chunk:	
	chunk.write(rng)
	
"""Note: Check the youtube video below for more information on how the procedural terrain generation should work for open areas and also for indoor areas.
https://youtu.be/GcM9Ynfzll0?t=10m9s"""

