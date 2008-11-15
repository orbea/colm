/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _RAGEL_H
#define _RAGEL_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "vector.h"
#include "config.h"
#include "common.h"

#define PROGNAME "ragel"

/* To what degree are machine minimized. */
enum MinimizeLevel {
	MinimizeApprox,
	MinimizeStable,
	MinimizePartition1,
	MinimizePartition2
};

enum MinimizeOpt {
	MinimizeNone,
	MinimizeEnd,
	MinimizeMostOps,
	MinimizeEveryOp
};

/* Options. */
extern MinimizeLevel minimizeLevel;
extern MinimizeOpt minimizeOpt;
extern const char *machineSpec, *machineName;
extern bool printStatistics;
extern bool wantDupsRemoved;
extern bool generateDot;

/* Error reporting format. */
enum ErrorFormat {
	ErrorFormatGNU,
	ErrorFormatMSVC,
};

extern ErrorFormat errorFormat;
extern int gblErrorCount;
extern char mainMachine[];

InputLoc makeInputLoc( const char *fileName, int line = 0, int col = 0 );
std::ostream &operator<<( std::ostream &out, const InputLoc &loc );

/* Error reporting. */
std::ostream &error();
std::ostream &error( const InputLoc &loc ); 
std::ostream &warning( const InputLoc &loc ); 

struct XmlParser;

void terminateAllParsers( );
void xmlEscapeHost( std::ostream &out, char *data, long len );

typedef Vector<const char *> ArgsVector;
extern ArgsVector includePaths;

extern CodeStyleEnum codeStyle;

/* IO filenames and stream. */
extern bool graphvizDone;

/* Options. */
extern int numSplitPartitions;
extern bool noLineDirectives;

std::ostream &error();

/* Target language and output style. */
extern CodeStyleEnum codeStyle;

/* Io globals. */
extern std::istream *inStream;
extern std::ostream *outStream;
extern output_filter *outFilter;
extern const char *outputFileName;

/* Graphviz dot file generation. */
extern bool graphvizDone;

extern int numSplitPartitions;
extern bool noLineDirectives;


#endif /* _RAGEL_H */
