/* land.c: LAND (COLLECTION OF ADDRESS RANGES) IMPLEMENTATION
 *
 * $Id: //info.ravenbrook.com/project/mps/branch/2014-03-30/land/code/land.c#1 $
 * Copyright (c) 2014 Ravenbrook Limited.  See end of file for license.
 *
 * .design: <design/land/>
 */

#include "mpm.h"
#include "range.h"

SRCID(land, "$Id$");


Bool LandCheck(Land land)
{
  CHECKS(Land, land);
  CHECKU(Arena, land->arena);
  CHECKL(AlignCheck(land->alignment));
  return TRUE;
}

Res LandInit(Land land, LandClass class, Arena arena, Align alignment, void *owner, ArgList args)
{
  Res res;

  AVER(land != NULL);
  AVERT(LandClass, class);
  AVER(AlignCheck(alignment));
  
  land->alignment = alignment;
  land->arena = arena;
  land->class = class;
  land->sig = LandSig;

  AVERT(Land, land);

  res = (*class->init)(land, args);
  if (res != ResOK)
    goto failInit;

  EVENT2(LandInit, land, owner);
  return ResOK;

 failInit:
  land->sig = SigInvalid;
  return res;
}

Res LandCreate(Land *landReturn, Arena arena, LandClass class, Align alignment, void *owner, ArgList args)
{
  Res res;
  Land land;
  void *p;

  AVER(landReturn != NULL);
  AVERT(Arena, arena);
  AVERT(LandClass, class);

  res = ControlAlloc(&p, arena, class->size,
                     /* withReservoirPermit */ FALSE);
  if (res != ResOK)
    goto failAlloc;
  land = p;

  res = LandInit(land, class, arena, alignment, owner, args);
  if (res != ResOK)
    goto failInit;

  *landReturn = land;
  return ResOK;

failInit:
  ControlFree(arena, land, class->size);
failAlloc:
  return res;
}

void LandDestroy(Land land)
{
  Arena arena;
  LandClass class;

  AVERT(Land, land);
  arena = land->arena;
  class = land->class;
  AVERT(LandClass, class);
  LandFinish(land);
  ControlFree(arena, land, class->size);
}

void LandFinish(Land land)
{
  AVERT(Land, land);
  (*land->class->finish)(land);
}

Res LandInsert(Range rangeReturn, Land land, Range range)
{
  AVER(rangeReturn != NULL);
  AVERT(Land, land);
  AVERT(Range, range);
  AVER(RangeIsAligned(range, land->alignment));

  return (*land->class->insert)(rangeReturn, land, range);
}

Res LandDelete(Range rangeReturn, Land land, Range range)
{
  AVER(rangeReturn != NULL);
  AVERT(Land, land);
  AVERT(Range, range);
  AVER(RangeIsAligned(range, land->alignment));

  return (*land->class->delete)(rangeReturn, land, range);
}

void LandIterate(Land land, LandVisitor visitor, void *closureP, Size closureS)
{
  AVERT(Land, land);
  AVER(FUNCHECK(visitor));

  (*land->class->iterate)(land, visitor, closureP, closureS);
}

Bool LandFindFirst(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  AVER(SizeIsAligned(size, land->alignment));
  AVER(FindDeleteCheck(findDelete));

  return (*land->class->findFirst)(rangeReturn, oldRangeReturn, land, size,
                                   findDelete);
}

Bool LandFindLast(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  AVER(SizeIsAligned(size, land->alignment));
  AVER(FindDeleteCheck(findDelete));

  return (*land->class->findLast)(rangeReturn, oldRangeReturn, land, size,
                                  findDelete);
}

Bool LandFindLargest(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  AVER(SizeIsAligned(size, land->alignment));
  AVER(FindDeleteCheck(findDelete));

  return (*land->class->findLargest)(rangeReturn, oldRangeReturn, land, size,
                                     findDelete);
}

Res LandFindInZones(Range rangeReturn, Range oldRangeReturn, Land land, Size size, ZoneSet zoneSet, Bool high)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  AVER(SizeIsAligned(size, land->alignment));
  /* AVER(ZoneSetCheck(zoneSet)); */
  AVER(BoolCheck(high));

  return (*land->class->findInZones)(rangeReturn, oldRangeReturn, land, size,
                                     zoneSet, high);
}

Res LandDescribe(Land land, mps_lib_FILE *stream)
{
  Res res;

  if (!TESTT(Land, land)) return ResFAIL;
  if (stream == NULL) return ResFAIL;

  res = WriteF(stream,
               "Land $P {\n", (WriteFP)land,
               "  class $P", (WriteFP)land->class,
               " (\"$S\")\n", land->class->name,
               "  arena $P\n", (WriteFP)land->arena,
               "  align $U\n", (WriteFU)land->alignment,
               NULL);
  if (res != ResOK)
    return res;

  res = (*land->class->describe)(land, stream);
  if (res != ResOK)
    return res;

  res = WriteF(stream, "} Land $P\n", (WriteFP)land, NULL);
  return ResOK;
}


Bool LandClassCheck(LandClass class)
{
  CHECKL(ProtocolClassCheck(&class->protocol));
  CHECKL(class->name != NULL); /* Should be <=6 char C identifier */
  CHECKL(class->size >= sizeof(LandStruct));
  CHECKL(FUNCHECK(class->init));
  CHECKL(FUNCHECK(class->finish));
  CHECKL(FUNCHECK(class->insert));
  CHECKL(FUNCHECK(class->delete));
  CHECKL(FUNCHECK(class->findFirst));
  CHECKL(FUNCHECK(class->findLast));
  CHECKL(FUNCHECK(class->findLargest));
  CHECKL(FUNCHECK(class->findInZones));
  CHECKL(FUNCHECK(class->describe));
  CHECKS(LandClass, class);
  return TRUE;
}


static Res landTrivInit(Land land, ArgList args)
{
  AVERT(Land, land);
  AVER(ArgListCheck(args));
  UNUSED(args);
  return ResOK;
}

static void landTrivFinish(Land land)
{
  AVERT(Land, land);
  NOOP;
}

static Res landNoInsert(Range rangeReturn, Land land, Range range)
{
  AVER(rangeReturn != NULL);
  AVERT(Land, land);
  AVERT(Range, range);
  return ResUNIMPL;
}

static Res landNoDelete(Range rangeReturn, Land land, Range range)
{
  AVER(rangeReturn != NULL);
  AVERT(Land, land);
  AVERT(Range, range);
  return ResUNIMPL;
}

static void landNoIterate(Land land, LandVisitor visitor, void *closureP, Size closureS)
{
  AVERT(Land, land);
  AVER(visitor != NULL);
  UNUSED(closureP);
  UNUSED(closureS);
  NOOP;
}

static Bool landNoFind(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  UNUSED(size);
  AVER(FindDeleteCheck(findDelete));
  return ResUNIMPL;
}

static Res landNoFindInZones(Range rangeReturn, Range oldRangeReturn, Land land, Size size, ZoneSet zoneSet, Bool high)
{
  AVER(rangeReturn != NULL);
  AVER(oldRangeReturn != NULL);
  AVERT(Land, land);
  UNUSED(size);
  UNUSED(zoneSet);
  AVER(BoolCheck(high));
  return ResUNIMPL;
}

static Res landTrivDescribe(Land land, mps_lib_FILE *stream)
{
  if (!TESTT(Land, land))
    return ResFAIL;
  if (stream == NULL)
    return ResFAIL;
  /* dispatching function does it all */
  return ResOK;
}

DEFINE_CLASS(LandClass, class)
{
  INHERIT_CLASS(&class->protocol, ProtocolClass);
  class->name = "LAND";
  class->size = sizeof(LandStruct);
  class->init = landTrivInit;
  class->finish = landTrivFinish;
  class->insert = landNoInsert;
  class->delete = landNoDelete;
  class->iterate = landNoIterate;
  class->findFirst = landNoFind;
  class->findLast = landNoFind;
  class->findLargest = landNoFind;
  class->findInZones = landNoFindInZones;
  class->describe = landTrivDescribe;
  class->sig = LandClassSig;
  AVERT(LandClass, class);
}


/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2014 Ravenbrook Limited <http://www.ravenbrook.com/>.
 * All rights reserved.  This is an open source license.  Contact
 * Ravenbrook for commercial licensing options.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. Redistributions in any form must be accompanied by information on how
 * to obtain complete source code for this software and any accompanying
 * software that uses this software.  The source code must either be
 * included in the distribution or be available for no more than the cost
 * of distribution plus a nominal fee, and must be freely redistributable
 * under reasonable conditions.  For an executable file, complete source
 * code means the source code for all modules it contains. It does not
 * include source code for modules or files that typically accompany the
 * major components of the operating system on which the executable file
 * runs.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
