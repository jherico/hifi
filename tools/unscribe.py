import re
import sys
import time
import os
import json
from pathlib import Path
from threading import Lock

outputDir = 'D:/shaders'
inputDir = 'C:/git/hifi/libraries'

seenIncludes = {}

includeFiles = []

includeDirs = [
    'gpu',
    'graphics',
    'procedural',
    'render',
    'render-utils',
    'entities-renderer',
]

def resolveInclude(includeName):
    result = None
    for includeDir in includeDirs:
        testFile = os.path.join(inputDir, includeDir, 'src', includeName)
        if (os.path.exists(testFile)):
            result = os.path.abspath(testFile)
            break
        testFile = os.path.join(inputDir, includeDir, 'src', includeDir, includeName)
        if (os.path.exists(testFile)):
            result = os.path.abspath(testFile)
            break
    if result == None:
        raise ValueError('Could not find include file for ' + includeName)

    if not result in seenIncludes:
        seenIncludes[result] = True
        includeFiles.append(result)

    return result

def isShaderFile(filename):
    return filename.endswith('.slv') or filename.endswith('.slf')

def replaceFunction(shader):
    # find the innermost function
    funcBegin = re.compile(r'<@func(?!.*<@func)\s+(.*?)@>',  flags=re.MULTILINE | re.DOTALL)
    funcEnd = re.compile(r'<@endfunc@>', flags=re.MULTILINE)

    me = re.search(funcEnd, shader)
    if None == me:
        return [False, shader]

    post = shader[me.end():]
    pre = shader[:me.start()]

    mf = re.search(funcBegin, pre)
    if None == mf:
        print(pre)
        raise ValueError("invalid input shader")

    function = '#define ' + mf.group(1) + ' \\\n' + ' \\\n'.join(pre[mf.end():].splitlines())
    shader = pre[:mf.start()] +  function + post
    return [True, shader]



def processShader(shader):
    replaced = True
    while replaced:
        (replaced, shader) = replaceFunction(shader)

    shader = re.sub(r'<!.*!>', r'', shader, flags=re.MULTILINE | re.DOTALL)
    shader = re.sub(r'<@((?:el)?if)\s+(\S+)\s*==\s*(\S+)\s*@>', r'#\1 (\2 == \3)', shader)
    shader = re.sub(r'<@((?:el)?if)\s+not\s+(\S+)\s*@>', r'#\1 !defined(\2)', shader)
    shader = re.sub(r'<@((?:el)?if)\s+(\S+)\s*@>', r'#\1 defined(\2)', shader)
    shader = re.sub(r'<@def\s+(\S+)\s*@>', r'#define \1', shader)
    shader = re.sub(r'<@else@>', r'#else', shader)
    shader = re.sub(r'<@endif@>', r'#endif', shader)

    # process includes
    includeRe = re.compile(r'<@include\s+(\S+)\s*@>', flags=re.MULTILINE)
    m = re.search(includeRe, shader)
    while m != None:
        includeFile = m.group(1)
        resolveInclude(includeFile)
        shader = shader[:m.start()] + '#include <' + includeFile + '>' + shader[m.end():]
        m = re.search(includeRe, shader)


    m = re.search(r'<@.*?@>', shader, flags=re.MULTILINE | re.DOTALL)
    if (m != None):
        print(m.group(0))
        print(shader)
        raise ValueError("Remaining substitution")

    return shader


def getTargetFolder(folder):
    result = folder[len(inputDir):]
    result = outputDir + result
    if not os.path.exists(result): 
        os.makedirs(result) 
        
    return result

def getTargetFile(shaderFile):
    index = shaderFile.rfind('.')
    extension = shaderFile[index:]
    switcher = {
        '.slv': '.vert',
        '.h': '.h',
        '.slh': '.h',
        '.slf': '.frag',
    }
    result = shaderFile[:index] + switcher[extension]
    return result


def processShaderfile(folder, file):
    input = os.path.join(folder, file)
    print('Reading ' + input)
    with open(input, 'r') as f:
        shader = processShader(f.read())
    
    output = os.path.join(getTargetFolder(folder), getTargetFile(file))
    with open(output, 'w') as f:
        f.write(shader)
        print('wrote ' + output)
    



def main():
    processShaderfile('C:/git/hifi/libraries/graphics/src/graphics', 'MaterialTextures.slh')

    for root, subdirs, files in os.walk(inputDir):
        for filename in files:
            if isShaderFile(filename):
                processShaderfile(root, filename)

    while len(includeFiles) != 0:
        filename = includeFiles.pop()
        processShaderfile(os.path.dirname(filename), os.path.basename(filename))


main()
