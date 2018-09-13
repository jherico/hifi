

import re
import subprocess
import sys
import time
import os
import json
import argparse
from concurrent.futures import ThreadPoolExecutor
from argparse import ArgumentParser
from pathlib import Path
from threading import Lock

scribeArgs = None


class TextTemplate:
    args = None
    output = []
    
    def __init__(self, args):
        self.args = args

    def parse(self):
        return True

    def generate(self):
        for header in self.args.header:
            with open(header) as f:
                self.output.append(f.read())

        switcher = {
            'vert': 'GPU_VERTEX_SHADER',
            'frag': 'GPU_PIXEL_SHADER',
            'geom': 'GPU_GEOMETRY_SHADER',
            'comp': 'GPU_COMPUTE_SHADER',
        }
        self.output.append('#define {}'.format(switcher[self.args.type]))

        result = '\n'.join(self.output)
        self.args.output.write(result)

    def scribe(self):
        if self.parse(): self.generate()

def parseArguments():
    # if (srcFilename.empty()) {
    #     cerr << "Usage: shaderScribe [OPTION]... inputFilename" << endl;
    #     cerr << "Where options include:" << endl;
    #     cerr << "  -o filename: Send output to filename rather than standard output." << endl;
    #     cerr << "  -t targetName: Set the targetName used, if not defined use the output filename 'name' and if not defined use the inputFilename 'name'" << endl;
    #     cerr << "  -I include_directory: Declare a directory to be added to the includes search pool." << endl;
    #     cerr << "  -D varname varvalue: Declare a var used to generate the output file." << endl;
    #     cerr << "       varname and varvalue must be made of alpha numerical characters with no spaces." << endl;
    #     cerr << "  -listVars : Will list the vars name and value in the standard output." << endl;
    #     cerr << "  -showParseTree : Draw the tree obtained while parsing the source" << endl;
    # }

    parser = argparse.ArgumentParser(description='Scribe shaders')
    parser.add_argument('--output', '-o', nargs='?', type=argparse.FileType('w'), default=sys.stdout)
    parser.add_argument('--input', '-i', nargs='?', type=argparse.FileType('r'), default=sys.stdin)
    parser.add_argument('--define', '-D', type=str, nargs=2, action='append')
    parser.add_argument('--type', '-T', type=str, default='vert', action='store', choices=['vert','frag','geom', 'comp'],)
    parser.add_argument('--include', '-I', type=str, action='append')
    parser.add_argument('--header', '-H', type=str, action='append')
    if 1 == len(sys.argv):
        testArgs = [
            '-D', 'GLPROFILE', 'PC_GL', 
            '-T', 'vert', 
            '-I', 'C:/Users/bdavi/git/hifi/libraries/gpu/src/gpu/', 
            '-I', 'C:/Users/bdavi/git/hifi/libraries/gpu/src/', 
            '-H', 'C:/Users/bdavi/git/hifi/libraries/shaders/headers/410/header.glsl', 
            '-H', 'C:/Users/bdavi/git/hifi/libraries/shaders/headers/mono.glsl', 
            '-i', 'C:/Users/bdavi/git/hifi/libraries/gpu/src/gpu/DrawTexcoordRectTransformUnitQuad.slv', 
            '-o', 'C:/Users/bdavi/git/hifi/build_noui/libraries/shaders/shaders/gpu/410/DrawTexcoordRectTransformUnitQuad.vert'
        ]
        return parser.parse_args(testArgs)

    return  parser.parse_args()



def main():
    scribeArgs = parseArguments()
    scribe = TextTemplate(scribeArgs)
    scribe.scribe()

main()
