from turbogears.database import PackageHub
from sqlobject import *

hub = PackageHub("space_viewer")
__connection__ = hub

