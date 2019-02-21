import json


class CharacterAttributes:
    """The CharacterAttributes class can be used to create and access
    the attributes of the different characters. Could potentially be
    used to create the attributes for NPCs too.
    The arguments it accept are:
    
    - Name : The name the character will have, it could be any string.
    - id : An integer that will be used as an id for the character.
    - Race : The Character race. Can be any string.
    - Stats : A dictionary with a list of stats and values for each stat, can be any type of stat.
    - level : The level of the character or NPC.
    
    Usage:
    
    ``#Define the properties or attributes``\n
    Phil = CharacterAttributes('Phil', 1, "Human", {'atk':1, 'magic':5})
    
    ``#Get the character level``\n
    Phil.level # Gets his level
    
    ``#Get the character attack damage stat``\n
    Phil.stats['atk'] # Gets his attack


    And if you wanted, you could even throw all the characters into an array

    hero_list = [Phil, Hero('Orc', 2, 'Orc', {'Orc Power':1234}, 8)
    for hero in hero_list:
        print hero.level # prints 1 for phil, 8 for orc
        
    """
    def __init__(self, name, id, race, stats, level=1):
        """ Creates the Hero"""
        self.name = name # Sets the name
        self.id = id # Sets the ID
        self.race = race # Sets the race (orc or human or etc)
        self.stats = stats # Sets the stats
        self.level = level # Sets the level
        

with open('data/Metadata/races.json') as f:
    races = json.load(f)        

for race in races:
    char = CharacterAttributes(race['name'], race['integer_id'], race['race'], race['base_stats'], 1)
    
    print 'Character race: ',char.name  
    print 'Base Stats',char.stats
    