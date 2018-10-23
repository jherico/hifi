#!/usr/bin/python3 

import os
import sys
import platform
import subprocess
import time
import json
import argparse
import concurrent
import tempfile
from git import Git
from argparse import ArgumentParser

def executeSubprocess(processArgs, folder=None):
    restoreDir = None
    if folder != None:
        restoreDir = os.getcwd()
        os.chdir(folder)

    processResult = subprocess.run(processArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if (0 != processResult.returncode):
        raise RuntimeError('Call to "{}" failed.\n\narguments:\n{}\n\nstdout:\n{}\n\nstderr:\n{}'.format(
            processArgs[0],
            ' '.join(processArgs[1:]), 
            processResult.stdout.decode('utf-8'),
            processResult.stderr.decode('utf-8')))

    if restoreDir != None:
        os.chdir(restoreDir)


class VcpkgRepo:
    def __init__(self):
        self.url = 'https://github.com/jherico/vcpkg'
        self.commit = 'hifi'
        defaultBasePath = os.path.join(tempfile.gettempdir(), 'hifi', 'vcpkg')
        self.basePath =  os.getenv('HIFI_VCPKG_BASE', defaultBasePath)
        self.path = os.path.join(self.basePath, self.commit)
        self.exe = os.path.join(self.path, 'vcpkg')
        self.bootstrapScript = os.path.join(self.path, 'bootstrap-vcpkg')
        self.triplet = 'x64-'
        system = platform.system()

        if 'Windows' == system:
            self.exe += '.exe'
            self.bootstrapScript += '.bat'
        else:
            self.bootstrapScript += '.sh'

        if 'Windows' == system:
            self.triplet += 'windows'
        elif 'Darwin' == system:
            self.triplet += 'osx'
        else:
            self.triplet += 'linux'


    def bootstrap(self):
        if (not os.path.isdir(self.path)):
            if not os.path.isdir(self.basePath):
                os.makedirs(self.basePath)
            Git(self.basePath).clone(self.url, self.commit)
        Git(self.path).checkout(self.commit)
        if not os.path.isfile(self.exe):
            executeSubprocess([self.bootstrapScript], folder=self.path)

    def run(self, commands):
        actualCommands = [self.exe, '--triplet', self.triplet, '--vcpkg-root', self.path]
        actualCommands.extend(commands)
        executeSubprocess(actualCommands, folder=self.path)

    def buildDependencies(self):
        # FIXME allow command line options to override the target
        self.run(['install', 'hifi-client-deps'])

    def write(self):
        cmakeScript = os.path.join(self.path, 'scripts/buildsystems/vcpkg.cmake').replace('\\', '/')
        vcpkgInstallPath = os.path.join(self.path, 'installed', self.triplet).replace('\\', '/')
        with open('vcpkg.cmake', 'w') as f:
            f.write('set(CMAKE_TOOLCHAIN_FILE "{}" CACHE FILEPATH "Toolchain file")\n'.format(cmakeScript))
            f.write('set(VKPKG_INSTALL_ROOT "{}" CACHE FILEPATH "vcpkg installed packages path")\n'.format(vcpkgInstallPath))

def main():
    vcpkg = VcpkgRepo()
    print("Verify VCPKG")
    vcpkg.bootstrap()
    print("Verify Hifi Dependencies")
    vcpkg.buildDependencies()
    print("Write CMake configuration")
    vcpkg.write()

# parser = ArgumentParser(description='Generate shader artifacts.')
# parser.add_argument('--commands', type=argparse.FileType('r'), help='list of commands to execute')
# parser.add_argument('--spirv-binaries', type=str, help='location of the SPIRV binaries')
# parser.add_argument('--build-dir', type=str, help='The build directory base path')
# parser.add_argument('--source-dir', type=str, help='The root directory of the git repository')
# parser.add_argument('--scribe', type=str, help='The scribe executable path')
# parser.add_argument('--debug', action='store_true')
# parser.add_argument('--force', action='store_true', help='Ignore timestamps and force regeneration of all files')
# parser.add_argument('--dry-run', action='store_true', help='Report the files that would be process, but do not output')

# args = None
# if len(sys.argv) == 1:
#     # for debugging
#     spirvPath = os.environ['VULKAN_SDK'] + '/bin'
#     #spirvPath = expanduser('~//VulkanSDK/1.1.82.1/x86_64/bin')
#     sourceDir = expanduser('~/git/hifi')
#     buildPath = sourceDir + '/build_noui'
#     scribePath = buildPath + '/tools/scribe/Release/scribe'
#     commandsPath = buildPath + '/libraries/shaders/shadergen.txt'
#     shaderDir = buildPath + '/libraries/shaders'
#     testArgs = '--commands {} --spirv-binaries {} --scribe {} --build-dir {} --source-dir {}'.format(
#         commandsPath, spirvPath, scribePath, shaderDir, sourceDir
#     ).split()
#     #testArgs.append('--debug')
#     #testArgs.append('--force')
#     #testArgs.append('--dry-run')
#     args = parser.parse_args(testArgs)
# else:
#     args = parser.parse_args()

main()

