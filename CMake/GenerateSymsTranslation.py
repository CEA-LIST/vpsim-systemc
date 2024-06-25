#!/usr/bin/env python3

for x in ['NmRawSyms',]:
 with open(x) as f:
    newL=[]
    for line in f.readlines():
        try:
          x,y,z=line.split()
          if x and y != 'U':
              newL.append('%s %s\n'%(z,'VpsimNamespace_'+z))
        except:pass
    w=open('SymsTranslation','w')
    w.write(''.join(set(newL)))
    w.close()
