# Copyright (C) 2008-today The SG++ Project
# This file is part of the SG++ project. For conditions of distribution and
# use, please see the copyright notice provided with SG++ or at
# sgpp.sparsegrids.org

import glob
import os
import os.path
import sys
import re

# get all folders containing an "SConscript*" file
# path has to end with "/"
def getModules(ignoreFolders):

#     if path[-1] != '/':
#         path += '/'
    #path = '#/'
    path = ''
    suffix = '/SConscript'
    searchString = path + '*' + suffix
    modulePaths = glob.glob(searchString)
    modules = []
    languageSupport = []

    for modulePath in modulePaths:
        module = modulePath[:-len(suffix)]
        module = module[len(path):]
        if module in ignoreFolders:
            continue
        if module in ['jsgpp', 'pysgpp']:
          languageSupport.append(module)
          continue
        modules.append(module)
    return modules, languageSupport

# Definition of flags / command line parameters for SCons
#########################################################################

def multiParamConverter(s):
    return s.split(',')

# detour compiler output
def print_cmd_line(s, target, src, env):
    if env['VERBOSE']:
        sys.stdout.write(u'%s\n' % s)
    else:
        sys.stdout.write(u'.')
        sys.stdout.flush()
    if env['CMD_LOGFILE']:
        with open(env['CMD_LOGFILE'], 'a') as logFile:
            logFile.write('%s\n' % s)

#creates a Doxyfile containing proper module paths based on Doxyfile_template
def prepareDoxyfile(modules):
    '''Create Doxyfile(s) and overview-pages
    @param modules list of modules'''

    # create Doxyfile
    with open('Doxyfile_template', 'r') as doxyFileTemplate:
        with open('Doxyfile', 'w') as doxyFile:
            inputPaths = 'INPUT ='
            examplePaths = 'EXAMPLE_PATH ='
            imagePaths = 'IMAGE_PATH ='

            for moduleName in modules:
                inputPath = moduleName + '/'
                examplePath = moduleName + '/examples'
                imagePath = moduleName + '/doc/doxygen/images'

                #print os.path.join(os.getcwd(),inputPath)
                if os.path.exists(os.path.join(os.getcwd(),inputPath)):
                    inputPaths += " " + inputPath
                if os.path.exists(os.path.join(os.getcwd(),examplePath)):
                    examplePaths += " " + examplePath
                if os.path.exists(os.path.join(os.getcwd(),imagePath)):
                    imagePaths += " " + imagePath

            for line in doxyFileTemplate.readlines():
                if re.search(r'INPUT  .*', line):
                    doxyFile.write(re.sub(r'INPUT.*', inputPaths, line))
                elif re.search(r'EXAMPLE_PATH  .*', line):
                    doxyFile.write(re.sub(r'EXAMPLE_PATH.*', examplePaths, line))
                elif re.search(r'IMAGE_PATH  .*', line):
                    doxyFile.write(re.sub(r'IMAGE_PATH.*', imagePaths, line))
                else:
                    doxyFile.write(line)

    # create example menu page
    with open('base/doc/doxygen/examples.doxy', 'w') as examplesFile:
        examplesFile.write('/**\n')
        examplesFile.write('@page examples Examples\n\n')
        examplesFile.write('This is a collection of examples from all modules.\n')
        examplesFile.write('To add new examples, go to the respective folder module/doc/doxygen/\n')
        examplesFile.write('and add a new example file code_examples_NAME.doxy with doxygen-internal\n')
        examplesFile.write('name code_examples_NAME.\n\n')

        for moduleName in modules:
            for subpage in glob.glob(os.path.join(moduleName, 'doc', 'doxygen', 'code_examples_*.doxy')):
                examplesFile.write('- @subpage ' + (os.path.split(subpage)[-1])[:-5] + '\n')

        examplesFile.write('**/\n')

    # create module page
    with open('base/doc/doxygen/modules.doxy', 'w') as modulesFile:
        with open('base/doc/doxygen/modules.stub0', 'r') as stubFile:
            modulesFile.write(stubFile.read())

        for moduleName in modules:
            for subpage in glob.glob(os.path.join(moduleName, 'doc', 'doxygen', 'module_*.doxy')):
                modulesFile.write('- @subpage ' + os.path.splitext(os.path.split(subpage)[-1])[0] + '\n')

        with open('base/doc/doxygen/modules.stub1', 'r') as stubFile:
            modulesFile.write(stubFile.read())
