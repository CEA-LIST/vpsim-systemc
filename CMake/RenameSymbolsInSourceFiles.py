#!/usr/bin/env python3

import os, sys, math

###############################################################################
#Replacement parameters set statically
###############################################################################

#If DRYRUN is True then no replacement are operated but substitution patterns are shown for debug
DRYRUN = False
TARGET_PARALLELISM = 8

#number of Sedstitution patterns to be grouped altogether in a single command
#this increases the computation in every call to sed and thus the parallelism
#note that there are about 8000 patterns
NB_PATTERNS_PER_SED = 64

#number of files sent by Xarg to each sed for inplace substitution 
#note that 101 files are edited in python include (could be moved to parameter if need be)
NB_FILES = 101.
NB_FILE_PER_SED = math.ceil(NB_FILES/TARGET_PARALLELISM)
print("NB_FILE_PER_SED %f" % (NB_FILE_PER_SED))

###############################################################################
#Input arguments handling
###############################################################################

if len(sys.argv)!=3:
    sys.exit("Usage is RenameSymboldsInSourceFiles.py SymbolFilePath PathToFolderToBeRenamed")
SymbolFilePath = sys.argv[1]
PathToFolderToBeRenamed = sys.argv[2]

###############################################################################
#Get functions to be renamed
###############################################################################
used={}
co=0
reg=[]
with open(SymbolFilePath) as f:
    lines = f.readlines()
    for line in lines:
      try:
        x, y, z = line.split()
        if y!='U' and  z not in used: 
            reg.append(z)
        used[z]=True
        co+=1
      except: pass
reg=sorted(reg,key= lambda x: -len(x))


###############################################################################
#Apply renaming for all regs
###############################################################################

progressCounter=0
lastProgressPercent=0
totalLength=len(reg)

# an optimization builds the substitution patterns in SedPattern to perform multiple substitutions per pass
SedPattern =""

for r in reg:
    #build a SedPattern for every NB_PATTERNS_PER_SED to increase computing per sed call
    if progressCounter%NB_PATTERNS_PER_SED==0 :
        #start pattern list
        SedPattern="'"
    else :
        #add pattern separator
        SedPattern+=";"

    #add a substitution pattern for every r
    #SedPattern += "s/\\b\(" + r + "\)\\b/VpsimNamespace_\\1/g"
    SedPattern += "s/\\b("+r+")\\b(?!\.h\\b)/VpsimNamespace_$1/g"

    if progressCounter%NB_PATTERNS_PER_SED==NB_PATTERNS_PER_SED-1 : 
        #close pattern and call sed
        SedPattern+="'"
        if DRYRUN:
            print("DRYRUN : sed pattern " + SedPattern)
        else :
            os.system("""find %s -type f -print0 | xargs -0 --max-procs=0 -L %d perl -i -pe %s """ % (PathToFolderToBeRenamed, NB_FILE_PER_SED, SedPattern))
            #os.system("""find %s -type f -print0 | xargs -0 --max-procs=0 -L %d sed -i %s """ % (PathToFolderToBeRenamed, NB_FILE_PER_SED, SedPattern))
            #os.system("""find %s -type f -print0 | xargs -0 --max-procs=0 -L 16 sed -i 's/\\b\(%s\)\\b/VpsimNamespace_\\1/g'""" % (PathToFolderToBeRenamed, r))
            #-t in xargs stands for print the command before its execution --> debug
            #-L 16 sends input files by groups of 16 to the sed command
            #--max-procs=0 stands for use as many parallel threads as possible to process the work 
            # together these options effectively parallelizes the work into 102/16 processors for every substitutions
    #very slow process so show progress to the user
    progressCounter+=1
    progressPercent= progressCounter*100/totalLength
    if int(progressPercent) != int(lastProgressPercent) :
        lastProgressPercent=progressPercent
        print(str(int(lastProgressPercent))+"%")

#need to handle special closing case after the loop
if progressCounter%NB_PATTERNS_PER_SED!=NB_PATTERNS_PER_SED-1 :
    SedPattern+="'"
    if DRYRUN:
        print("DRYRUN : sed pattern " + SedPattern)
    else :
        os.system("""find %s -type f -print0 | xargs -0 --max-procs=0 -L %d perl -i -pe %s """ % (PathToFolderToBeRenamed, NB_FILE_PER_SED, SedPattern))
        #os.system("""find %s -type f -print0 | xargs -0 --max-procs=0 -L %d sed -i %s """ % (PathToFolderToBeRenamed, NB_FILE_PER_SED, SedPattern))
