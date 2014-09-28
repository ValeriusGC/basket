# -*- coding: utf-8 -*-
"""
Created on Sun Sep 28 17:41:58 2014
@function Gets all used icons and deletes unused from oxygen set
@author: keelan
"""

import subprocess
import os
import re

bashCommand = "grep QIcon::fromTheme . -R"
process = subprocess.Popen(bashCommand.split(), stdout=subprocess.PIPE)
output = process.communicate()[0]

stringList = output.decode("utf-8").split('\n')

usedIcons = []
for line in stringList:
    m = re.search('fromTheme\(\"([a-z\-]+)', line)
    if(m):
        usedIcons.append(m.group(1))
        
usedIcons = list(set(usedIcons)) # remove duplicates

bashCommand = "grep . ./oxygen/ -R"
process = subprocess.Popen(bashCommand.split(), stdout=subprocess.PIPE)
output = process.communicate()[0]
stringList = output.decode("utf-8").split('\n')

currentIcons = {}
for line in stringList:
    m1 = re.search('/([A-za-z0-9\-\.\+]+).png', line)
    m2 = re.search('/.+/[A-za-z0-9\-\.\+]+.png', line)
    if(m1 and m2):
        currentIcons[m1.group(1)] = "/home/keelan/Google Drive/Programming/Qt/basket-qmake/basket" + m2.group(0)

for key in currentIcons:
    if key not in usedIcons:
        print(key + " - " + currentIcons[key])
        try:
            os.remove(currentIcons[key])
        except OSError:
            print("Failed" + currentIcons[key])