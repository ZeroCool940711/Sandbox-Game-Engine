#!/usr/bin/python2.3

# CVS keyword conflict fixer-upper

import sys
file = sys.stdin.readlines()
flen = len(file)
fixup = -1
for i in range(flen-5):
	if file[i].find("<<<<<<<") < 0: continue
	# check for dollar-Id conflicts
	if	file[i+1].find("$Id")>=0 and \
		file[i+2].find("=======")>=0 and \
		file[i+3].find("$Id")>=0 and \
		file[i+4].find(">>>>>>>")>=0:
			fixup = i
if fixup != -1:
	del file[fixup+2:fixup+5];
	del file[fixup];


fixup1 = -1
fixup2 = -1
fixup3 = -1
state = 0
for i in range(flen-5):
	if state == 0:
		if file[i].find("<<<<<<<") < 0: continue
		# check for dollar-Log conflicts
		if file[i+1].find("$Log")>=0:
			fixup1 = i
			state = 1
	elif state == 1:
		if file[i].find("=======") < 0: continue
		if file[i+1].find("$Log")>=0:
			fixup2 = i
			state = 2
		else:
			state = 0
	elif state == 2:
		if file[i].find(">>>>>>>") < 0: continue
		fixup3 = i
		state = 3
		break

if state == 3:
	del file[fixup3]
	newlog=file[fixup2+2:fixup3]
	del file[fixup2:fixup3]
	file[fixup1+2:fixup1+2] = newlog
	del file[fixup1]

sys.stdout.writelines(file)


