import json, random
from numpy.random import choice

def get_item_base_type(level, count):
    with open('data/Metadata/base_items.json') as f:
        data = json.load(f)
    
    for items in data.keys():
        if not data[items]['drop_level'] <= level:
            data.pop(items)
        elif data[items]['release_state'] == 'legacy':
            data.pop(items)
        elif 'bestiary_net' in data[items]['tags']:
            data.pop(items)
    
    choises = random.sample(data,count)
    
    drops = []
    
    for item in choises:
        drops.append(data[item])
        
    return drops
       
       
       
def get_rarity(count=1, player_iir=1, monster_rarity=1, party_bonus=1):
    ''' Items and monsters have rarities, the higher the rarity the stronger
        the items or monsters are.
        Here is a list of rarities starting from common to the most rare ones:

        - Inferior : Inferior monsters/items do not have mods or have a much weaker version of the normal mod.
        - Normal : Normal monsters/items have no mods, only some of them can have implicit or passive mods.
        - Magic
        - Rare
        - Unique*
        - Ancient*
        - Legendary*
        - Phantasmal*
        
        Note: Ancient, Legendary and Phantasmal are considered Artifacts or Relics.
        
        
        
        - Some items and monsters have special mods that make them stronger than what they normally are,
        starting from the magic rarity this type of mods can start to appear on items and monsters. 
        
        - Starting from the Unique rarity monsters and items have (as the name say) unique mods that
        combined with other mods from the same items/monster can be really powerful.
        
        - Ancient items have better mods and combination that work better than those on Unique items/monsters.
        
        - Legendary are better than Ancient and Phantasmal are really, really rare monsters and items that can
        be way more powerful than anything else that can be found, this type of items can change a whole build
        and are so rare that its hard to find phantasmal items with the same combination of mods, these
        can be found from monsters with the mentioned rarity and they only appear on Raids which are huge fights
        where multiple groups of 6 players each for a total of 30 players have to complete a huge dungeon with
        multiple bosses with Phantasmal rarity and special habilities in order to get them.

        '''
    rarity_list = ['Inferior', 'Normal', 'Magic', 'Rare', 'Unique', 'Ancient', 'Legendary', 'Phantasmal']
    weight = [0.4, 0.3, 0.2, 0.08, 0.01, 0.001, 0.00008, 0.000002]
    
    
    rarities = choice(rarity_list, count, weight)
    
    return rarities

print get_rarity(100)

#item_base = get_item_base_type(2, 10)

#for items in item_base:
    #print items['name']