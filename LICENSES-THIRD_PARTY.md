# VPSim: Third party contributions

VPSIM uses code contributions from various sources, mostly through dynamic 
linking to shared libraries.


The current file lists relevant third party contributions, attempts to give
attribution whenever possible and lists relevant licenses.

Futher full license text is available in the LICENSES subdirectories.

Please contact sesam-vpsim@cea.fr for clarifications or to suggest 
corrections in this file.

## `vpsim` executable

### Built-in contributions

The following libraries are used for building the `vpsim` executable.

#### SystemC Class Library Proof of Concept implementation

The VPSIM executable includes code from the Accelera SystemC implementation 
version 2.3.2 available from the Accelera website (accellera.org), currently at 
 https://accellera.org/images/downloads/standards/systemc/systemc-2.3.2.tar.gz

```
SPDX-License-Identifier: Apache-2.0
```

Full license text in `LICENSES/APACHE_LICENSE-2.0.txt`


```
/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

 *****************************************************************************/

=========================================================================
==  NOTICE file corresponding to section 4 d of the Apache License,    ==
==  Version 2.0, in this case for the SystemC Class Library            ==
==  Proof of Concept implementation                                    ==
=========================================================================
 
 This product includes software developed by Accellera Systems Initiative
 8698 Elk Grove Bldv Suite 1, #114, Elk Grove, CA 95624, USA
 Copyright 2011-2016 Accellera Systems Initiative Inc. (Accellera) 
 All rights reserved.

 This product includes software developed by Cadence Design Systems Inc.
 2655 Seely Ave., San Jose, CA, USA
 Copyright 2006-2016 Cadence Design Systems Inc.
 All rights reserved.

 This product includes software developed by Circuitsutra Technologies Pvt Ltd.
 Regus, Ground Floor, Tapasya Corp Heights, Sector 126, Noida, 201303, UP, India
 Copyright 2011-2016 Circuitsutra Technologies Pvt Ltd.
 All rights reserved.

 This product includes software developed by Doulos
 Church Hatch, 22 Market Place, Ringwood, Hampshire, BH24 1AW, United Kingdom
 Copyright 2006-2016 Doulos
 All rights reserved.

 This product includes software developed by Fraunhofer-Gesellschaft 
 Postfach 20 07 33, 80007 Munich, Germany 
 Copyright 2014-2016 Fraunhofer-Gesellschaft
 All rights reserved.

 This product includes software developed by Intel Corp.
 2200 Mission College Blvd., Santa Clara, CA 95054-1549, USA
 Copyright 2007-2016 Intel Corp.
 All rights reserved. 

 This product includes software developed by Mentor Graphics Corporation
 8005 SW Boeckman Road, Wilsonville, OR 97070, USA
 Copyright 2006-2016 Mentor Graphics Corporation
 All rights reserved.

 This product includes software developed by OFFIS eV
 Escherweg 2, 26121 Oldenburg, Germany
 Copyright 2009-2016 OFFIS eV
 All rights reserved. 

 This product includes software developed by STMicroelectronics International NV
 39, Chemin de Champ-des-Filles, Plan-Les-Ouates, Geneva, Switzerland
 Copyright 2011-2016 STMicroelectronics International NV
 All rights reserved. 

 This product includes software developed by Synopsys, Inc.
 690 East Middlefield Road Mountain View, CA 94043, USA
 Copyright 1999-2016 Synopsys, Inc.
 All rights reserved. 

 This product includes software developed by Universite Pierre et Marie Curie
 b.c. 167, 4 place Jussieu, 75252 Paris, France
 Copyright 2011-2016 Universite Pierre et Marie Curie (UPMC) 
 All rights reserved. 

 This product includes software developed by XtremeEDA Corporation
 201-1339 Wellington St. West, Ottowa, Ontario K1Y3B8, Canada
 Copyright 2011-2016 XtremeEDA Corporation. All rights reserved.

```

#### python3

Python3 is embedded in the vpsim executable.


```
SPDX-License-Identifier: PSF-2.0
```

As requested by clause (3.) below, a brief summary of changes:

  Python sources are unmodified and compiled 
  to be included as relocatable code.


```
Python software and documentation are licensed under the
Python Software Foundation License Version 2.

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021 Python Software Foundation;
All Rights Reserved

PYTHON SOFTWARE FOUNDATION LICENSE VERSION 2
--------------------------------------------

1. This LICENSE AGREEMENT is between the Python Software Foundation
("PSF"), and the Individual or Organization ("Licensee") accessing and
otherwise using this software ("Python") in source or binary form and
its associated documentation.

2. Subject to the terms and conditions of this License Agreement, PSF hereby
grants Licensee a nonexclusive, royalty-free, world-wide license to reproduce,
analyze, test, perform and/or display publicly, prepare derivative works,
distribute, and otherwise use Python alone or in any derivative version,
provided, however, that PSF's License Agreement and PSF's notice of copyright,
i.e., "Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021 Python Software Foundation;
All Rights Reserved" are retained in Python alone or in any derivative version
prepared by Licensee.

3. In the event Licensee prepares a derivative work that is based on
or incorporates Python or any part thereof, and wants to make
the derivative work available to others as provided herein, then
Licensee hereby agrees to include in any such work a brief summary of
the changes made to Python.

4. PSF is making Python available to Licensee on an "AS IS"
basis.  PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND
DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON WILL NOT
INFRINGE ANY THIRD PARTY RIGHTS.

5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON,
OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.

6. This License Agreement will automatically terminate upon a material
breach of its terms and conditions.

7. Nothing in this License Agreement shall be deemed to create any
relationship of agency, partnership, or joint venture between PSF and
Licensee.  This License Agreement does not grant permission to use PSF
trademarks or trade name in a trademark sense to endorse or promote
products or services of Licensee, or any third party.

8. By copying, installing or otherwise using Python, Licensee
agrees to be bound by the terms and conditions of this License
Agreement.
```

#### elfio

```
SPDX-License-Identifier: MIT
```

```
ELFIO library implemented by
  Serge Lamikhov-Center <to_serge@users.sourceforge.net>

MIT License

Copyright (C) 2001-2011 by Serge Lamikhov-Center

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

#### moodycamel::ConcurrentQueue

```
SPDX-License-Identifier: BSD-2-Clause
```

https://github.com/cameron314/concurrentqueue

```
Simplified BSD License:

Copyright (c) 2013-2016, Cameron Desrochers.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

```

### Libraries (dynamically linked)

Dependencies on libc and libstdc++ under 
GNU General Public License v3.0 or later WITH GCC Runtime Library exception 3.1

GPLv3 text: ``LICENSES/GPLv3``

GCC Runtime Library extension text: ``LICENSES/GCC-RuntimeLibraryExceptions3.1``


```
SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1
```

https://www.gnu.org/licenses/gcc-exception-3.1.html

Also, in no particular order

#### Zlib

```
SPDX-License-Identifier: Zlib
```

```
zlib was written by Jean-loup Gailly (compression) and Mark Adler (decompression).
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
```

#### Unicode ( libicu )

```
SPDX-License-Identifier: ICU
```

Full text of licence in LICENSES/ICU


```
COPYRIGHT AND PERMISSION NOTICE (ICU 58 and later)

Copyright © 1991-2020 Unicode, Inc. All rights reserved.
Distributed under the Terms of Use in https://www.unicode.org/copyright.html.

Permission is hereby granted, free of charge, to any person obtaining
a copy of the Unicode data files and any associated documentation
(the "Data Files") or Unicode software and any associated documentation
(the "Software") to deal in the Data Files or Software
without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, and/or sell copies of
the Data Files or Software, and to permit persons to whom the Data Files
or Software are furnished to do so, provided that either
(a) this copyright and permission notice appear with all copies
of the Data Files or Software, or
(b) this copyright and permission notice appear in associated
Documentation.

THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS
NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL
DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THE DATA FILES OR SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in these Data Files or Software without prior
written authorization of the copyright holder.
```

#### double-conversion library

```
SPDX-License-Identifier: BSD-3-clause
```

```
Copyright: 2006-2008, 2010-2012 The V8 project authors

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
.
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

#### Glib 

https://gitlab.gnome.org/GNOME/glib/

```
SPDX-License-Identifier: LGPL-2.1+
```

Text of LGPL2.1 in LICENSES/LGPL2.1

```
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, see <http://www.gnu.org/licenses/>.
```

#### PCRE

http://www.pcre.org/

```
SPDX-License-Identifier: BSD-3-Clause
```

Full license text in LICENSES/PCRE

#### RapidXML

```
SPDX-License-Identifier: MIT
```

Copyright (c) 2006, 2007 Marcin Kalicinski

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
