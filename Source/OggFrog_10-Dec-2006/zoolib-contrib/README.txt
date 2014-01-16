The files in the zoolib-contrib directory are meant to be 
contributed to the ZooLib cross-platform application framework 
at http://www.zoolib.org/  All the source code in this directory 
has the MIT license, as does ZooLib.

However, what's here now is in a pretty half-baked state and isn't 
nearly ready for production use.  In particular, all of this source
code is meant to stand on its own, except where it depends on external
libraries like libvorbis and libogg, but with no dependencies on the
main Ogg Frog source code.  But ZPCMSink explicitly depends on OFApp
and #includes OFApp.h.  This dependency will be removed in the next
source release.

- Michael D. Crawford 
  rippit@oggfrog.com 
  http://www.oggfrog.com/