import hifi_utils
import hashlib
import os
import platform
import re
import shutil
import tempfile
import json
import xml.etree.ElementTree as ET

def generateQrc():
    root = ET.Element('RCC', { 'version':"1.0" })
    qresource = ET.SubElement(root, 'qresource', {'prefix':'/'})

    resourceFiles = []
    resourcesDir = hifi_utils.scriptRelative('interface/resources').replace('\\', '/')
    for dirName, subdirList, fileList in os.walk(resourcesDir):
        for fname in fileList:
            resourceFiles.append(os.path.join(dirName, fname).replace('\\', '/'))
    resourceFiles.sort()
    for fullpath in resourceFiles:
        alias = os.path.relpath(fullpath, resourcesDir).replace('\\', '/')
        ET.SubElement(qresource, 'file', {'alias': alias}).text = fullpath
    
    qrcContents = '<!DOCTYPE RCC>' + ET.dump(root)

    targetQrc = hifi_utils.scriptRelative('interface/compiledResources/resources.qrc')
    with open(targetQrc, 'w') as f:
        f.write(qrcContents)
