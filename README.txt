README.txt

BSPv30 Loader for Irrlicht

Aaron Saderholm
saderbiscut@gmail.com

This project should be able to run on anything that can run Irrlicht.  I am currently using a random
pull of the 1.8 development branch, but hypothetically it should work with 1.7 too.

To use this you need an irrlicht based project, and you need to include cBSP30.h  You will also need a version 30 BSP file.

However, this loader is currently incomplete and if you try to use it you 
are crazy.  It currently loads BSP files as well as textures them, but the short list of identifiable problems that needs to be fixed are as follows:

-It dosen't read any kind of entity data yet, this is a prerequisite for a couple of the following issues:

	-It only loads textures stored inside the BSP and not in refrenced WAD files.  WAD Data is stored in the world entity, and I think irrlicht has support for WAD files now, I just haven't gotten around to implementing it.
	-I forget if it either loads everything as one model or just loads the entire world model, but either way, there isn't a mechanism for seperating entity models which makes them somewhat useless.
	-It dosen't handle special texture effects or transparent textures.
	
-This project either has terrible/no memory management, I'm pretty sure it's a giant memory leak when it comes to how it stores textures.  It dosen't clean up after itself and probably dosen't even allocate memory correctly in the first place.

-It has pretty awful performance in large maps.  The default occulsion culling causes issues, so I have it turned off.  It also dosen't implement VIS Nodes as I hadn't figure out a good way to do those with irrlicht.  The answer to the aformentioned performance problems is probably here.

-It dosen't have lighting or lightmaps.  I would like lightmaps very much but I am so far confused as to both how BSPv30 implements them but also how they would work with Irrlicht's lighting as I basicly have no idea how the latter works.

In essence, I am dumb and have no clue what I am doing, but I did it anyway!  If you find an instance where I did something horribly backwards, you're probably correct and you're welcome to fix it or email me with helpful advice, which is always appriceated.



Portions of this project are taken from the irrlicht engine, copyright Nikolaus Gebhardt.  

For more information see: http://irrlicht.sourceforge.net/license/

This project is released under the MIT Licence and as such you can do whatever you want with it.  I would prefer that any further developments are also released in such an open fashion for use by the public, but obviously, this is not required.


	Copyright (c) 2013 Aaron Saderholm

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Aaron