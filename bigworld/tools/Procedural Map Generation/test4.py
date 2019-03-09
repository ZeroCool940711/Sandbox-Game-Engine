#import matplotlib.pyplot as plt
#import numpy as np

## This import registers the 3D projection, but is otherwise unused.
#from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 unused import

## prepare some coordinates
#x, y, z = np.indices((5, 5, 5))

#print x

## draw cuboids in the top left and bottom right corners, and a link between them
#voxels = (x < 5) & (y < 5) & (z < 5)

## set the colors of each object
#colors = np.empty(voxels.shape, dtype=object)
#colors[voxels] = 'blue'

## and plot everything
#fig = plt.figure()
#ax = fig.gca(projection='3d')
#ax.voxels(voxels, facecolors=colors, edgecolor='k')

#plt.show()

#import pyvoro
#a = pyvoro.compute_voronoi(
    #[[1.0, 2.0, 3.0], [4.0, 5.5, 6.0]], # point positions
    #[[0.0, 10.0], [0.0, 10.0], [0.0, 10.0]], # limits
    #2.0, # block size
    #radii=[1.3, 1.4] # particle radii -- optional, and keyword-compatible arg.
#)

#print a

import os
import sys
from PyQt4.QtGui import *

# Create window
app = QApplication(sys.argv)
w = QWidget()
w.setWindowTitle("Image Node")

# Create widget
label = QLabel(w)
pixmap = QPixmap('DoubleBasin_big.png')
label.setPixmap(pixmap)
w.resize(pixmap.width(),pixmap.height())

# Draw window
w.show()
app.exec_()
