#
# Simple setup script for py2exe to compile cvs_stub.py into an executable.
#

from distutils.core import setup
import py2exe

setup(console=['cvs_stub.py'])