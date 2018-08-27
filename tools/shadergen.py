import re
import subprocess
import sys
import time
import os
import json
import argparse
import concurrent

from concurrent.futures import ThreadPoolExecutor
from argparse import ArgumentParser
from pathlib import Path
from threading import Lock

    # # Target dependant Custom rule on the SHADER_FILE
    # if (ANDROID)
    #     set(GLPROFILE LINUX_GL)
    # else() 
    #     if (APPLE)
    #         set(GLPROFILE MAC_GL)
    #     elseif(UNIX)
    #         set(GLPROFILE LINUX_GL)
    #     else()
    #         set(GLPROFILE PC_GL)
    #     endif()
    # endif()

def getTypeForScribeFile(scribefilename):
    last = scribefilename.rfind('.')
    extension = scribefilename[last:]
    switcher = {
        '.slv': 'vert',
        '.slf': 'frag',
        '.slg': 'geom',
        '.slc': 'comp',
    }
    if not extension in switcher:
        sys.exit("Unknown scribe file type for " + scribefilename)
    return switcher.get(extension)

def getCommonScribeArgs(scribefile, includeLibs):
    scribeArgs = [args.scribe]
    # FIXME use the sys.platform to set the correct value
    scribeArgs.extend(['-D', 'GLPROFILE', 'PC_GL'])
    scribeArgs.extend(['-T', getTypeForScribeFile(scribefile)])
    for lib in includeLibs:
        scribeArgs.extend(['-I', args.source_dir + '/libraries/' + lib + '/src/' + lib + '/'])
        scribeArgs.extend(['-I', args.source_dir + '/libraries/' + lib + '/src/'])
    scribeArgs.append(scribefile)
    return scribeArgs

def getDialectAndVariantHeaders(dialect, variant):
    headerPath = args.source_dir + '/libraries/shaders/headers/'
    variantHeader = headerPath + ('stereo.glsl' if (variant == 'stereo') else 'mono.glsl')
    dialectHeader = headerPath + dialect + '/header.glsl'
    return [dialectHeader, variantHeader]

class ScribeDependenciesCache:
    cache = {}
    lock = Lock()
    filename = ''

    def __init__(self, filename):
        self.filename = filename

    def load(self):
        jsonstr = '{}'
        if (os.path.exists(self.filename)):
            with open(self.filename) as f:
                jsonstr = f.read()
        self.cache = json.loads(jsonstr)

    def save(self):
        with open(self.filename, "w") as f:
            f.write(json.dumps(self.cache))

    def get(self, scribefile):
        self.lock.acquire()
        try:
            if scribefile in self.cache:
                return self.cache[scribefile].copy()
        finally:
            self.lock.release()
        return None

    def getOrGen(self, scribefile, includeLibs, dialect, variant):
        result = self.get(scribefile)
        if (None == result):
            result = self.gen(scribefile, includeLibs, dialect, variant)
        return result

    def gen(self, scribefile, includeLibs, dialect, variant):
        scribeArgs = getCommonScribeArgs(scribefile, includeLibs)
        scribeArgs.extend(['-M'])
        processResult = subprocess.run(scribeArgs, stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to parse scribe dependencies")
        result = processResult.stdout.decode("utf-8").splitlines(False)
        result.append(scribefile)
        result.extend(getDialectAndVariantHeaders(dialect, variant))
        self.lock.acquire()
        self.cache[scribefile] = result.copy()
        self.lock.release()
        return result


scribeDepCache = None

def getFileTimes(files):
    if isinstance(files, str):
        files = [files]
    return list(map(lambda f: os.path.getmtime(f) if os.path.isfile(f) else -1, files))

def outOfDate(inputs, output):
    latestInput = max(getFileTimes(inputs))
    earliestOutput = min(getFileTimes(output))
    return latestInput >= earliestOutput


def processCommand(line):
    global args
    glslangExec = args.spirv_binaries + '/glslangValidator'
    spirvCrossExec = args.spirv_binaries + '/spirv-cross'
    spirvOptExec = args.spirv_binaries + '/spirv-opt'
    params = line.split(';')
    dialect = params.pop(0)
    variant = params.pop(0)
    scribeFile  = args.source_dir + '/' + params.pop(0)
    unoptGlslFile = args.source_dir + '/' + params.pop(0)
    libs = params

    upoptSpirvFile = unoptGlslFile + '.spv'
    spirvFile = unoptGlslFile + '.opt.spv'
    reflectionFile  = unoptGlslFile + '.json'
    glslFile = unoptGlslFile + '.glsl'
    outputFiles = [unoptGlslFile, spirvFile, reflectionFile, glslFile]

    scribeOutputDir = os.path.abspath(os.path.join(unoptGlslFile, os.pardir))
    if not os.path.exists(scribeOutputDir):
        os.makedirs(scribeOutputDir) 
        
    scribeDeps = scribeDepCache.getOrGen(scribeFile, libs, dialect, variant)

    # if the scribe sources (slv, slf, slh, etc), or the dialect/ variant headers are out of date
    # regenerate the scribe GLSL output
    if outOfDate(scribeDeps, outputFiles):
        print('Processing file {} dialect {} variant {}'.format(scribeFile, dialect, variant))
        scribeDepCache.gen(scribeFile, libs, dialect, variant)
        scribeArgs = getCommonScribeArgs(scribeFile, libs)
        for header in getDialectAndVariantHeaders(dialect, variant):
            scribeArgs.extend(['-h', header])
        scribeArgs.extend(['-o', unoptGlslFile])
        processResult = subprocess.run(scribeArgs, stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to generate scribe output " + unoptGlslFile)
        # Generate the un-optimized output
        glslangArgs = [glslangExec, '-V110', '-o', upoptSpirvFile, unoptGlslFile]
        processResult = subprocess.run(glslangArgs, stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to generate spirv output " + upoptSpirvFile)

        # Optimize the SPIRV
        spirvOptArgs = [spirvOptExec, '-O', '-o', spirvFile, upoptSpirvFile]
        processResult = subprocess.run(spirvOptArgs, stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to generate optimized spirv output " + spirvFile)

        # Generation JSON reflection
        processResult = subprocess.run(
            [spirvCrossExec, '--reflect', 'json', '--output', reflectionFile, spirvFile], 
            stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to generate spirv reflection " + reflectionFile)

        spirvCrossArgs = [spirvCrossExec, '--output', glslFile, spirvFile, '--version', dialect]
        if (dialect == '410'): spirvCrossArgs.append('--no-420pack-extension')
        processResult = subprocess.run(spirvCrossArgs, stdout=subprocess.PIPE)
        if (0 != processResult.returncode):
            sys.exit("Unable to generate spirv reflection " + glslFile)
    else:
        Path(unoptGlslFile).touch()
        Path(upoptSpirvFile).touch()
        Path(spirvFile).touch()
        Path(glslFile).touch()
        Path(reflectionFile).touch()



def main():
    commands = args.commands.read().splitlines(False)
    if args.debug:
        for command in commands:
            processCommand(command)
    else:
        workers = max(1, os.cpu_count() - 2)
        with ThreadPoolExecutor(max_workers=workers) as executor:
            executor.map(processCommand, commands)
            executor.shutdown()

parser = ArgumentParser(description='Generate shader artifacts.')
parser.add_argument('--commands', type=argparse.FileType('r'), help='list of commands to execute')
parser.add_argument('--spirv-binaries', type=str, help='location of the SPIRV binaries')
parser.add_argument('--build-dir', type=str, help='The build directory base path')
parser.add_argument('--source-dir', type=str, help='The root directory of the git repository')
parser.add_argument('--scribe', type=str, help='The scribe executable path')
parser.add_argument('-d', '--debug', action='store_true')

args = None
if len(sys.argv) == 1:
    # for debugging
    spirvPath = '/home/bdavis/VulkanSDK/1.1.82.1/x86_64/bin'
    sourceDir = '/home/bdavis/git/hifi'
    buildPath = sourceDir + '/build'
    scribePath = buildPath + '/tools/scribe/scribe'
    commandsPath = buildPath + '/libraries/shaders/shadergen.txt'
    shaderDir = buildPath + '/libraries/shaders'
    testArgs = '--commands {} --spirv-binaries {} --scribe {} --build-dir {} --source-dir {}'.format(
        commandsPath, spirvPath, scribePath, shaderDir, sourceDir
    ).split()
    args = parser.parse_args(testArgs)
else:
    args = parser.parse_args()

scribeDepCache = ScribeDependenciesCache(args.build_dir + '/shaderDeps.json')
scribeDepCache.load()
main()
scribeDepCache.save()

