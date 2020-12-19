# Reckon Makefile
#
# File:		Makefile
# Author:	Bob Walton (walton@acm.org)
# Date:		Sat Dec 19 06:36:31 EST 2020
#
# The authors have placed this program in the public
# domain; they make no warranty and accept no liability
# for this program.

# The following locates the MIN and LAYERED source code.
#
MIN = ~/min
LAYERED = ~/layered

# The following turns on MIN debugging.
#
GFLAGS = -DMIN_PROTECT=1
WFLAGS = -Wall -Wno-char-subscripts

.SUFFIXES:
.SUFFIXES: .out .test .diff

# O3 is needed to inline functions.
#
OPTIMIZE = -O3

RECKON_TESTS = reckon_lexeme_test reckon_bracketed_test \
	       reckon_snippet_test

TESTS = ${RECKON_TESTS}

.SECONDARY:	${TESTS:=.out}

all:		diff

reckon_diff:	${RECKON_TESTS:=.diff}
diff:		${TESTS:=.diff}

out:		${TESTS:=.out}

test:		${TESTS:=.test}


%.test:	%.out Makefile
	rm -f $*.test
	sed \
	    -e '/\.cc:[0-9]* assert/s//.cc:XXXX assert/' \
	    -e '/\.h:[0-9]* assert/s//.h:XXXX assert/' \
	    -e '/\.cc:[0-9]* desire/s//.cc:XXXX desire/' \
	    -e '/\.L/s/\(\.L[A-Z][A-Z]*\)[0-9][0-9]*/\1XXX/g' \
	    -e '/line [0-9]/s//line #/g' \
	    -e '/\(lines [0-9][0-9]*-\)[0-9]/s//\1#/g' \
	    -e '/lines [0-9]\([0-9]*-#\)/s//lines #\1/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    < $*.out > $*.test

%.diff:	%.out
	@echo DIFFING $*.test $*.out
	-@sed \
	    -e '/\.cc:[0-9]* assert/s//.cc:XXXX assert/' \
	    -e '/\.h:[0-9]* assert/s//.h:XXXX assert/' \
	    -e '/\.cc:[0-9]* desire/s//.cc:XXXX desire/' \
	    -e '/\.L/s/\(\.L[A-Z][A-Z]*\)[0-9][0-9]*/\1XXX/g' \
	    -e '/line [0-9]/s//line #/g' \
	    -e '/\(lines [0-9][0-9]*-\)[0-9]/s//\1#/g' \
	    -e '/lines [0-9]\([0-9]*-#\)/s//lines #\1/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/#[0-9]/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/[0-9]#/s//##/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    -e '/@[0-9]/s//@@/g' \
	    < $*.out | diff --minimal -u ${DIFF_FLAGS} $*.test -

%.out:	%
	rm -f $*.out
	${RUNENV} $* > $*.out

reckon_parser.o:            \
	    ../src/reckon_parser.cc \
	    ../include/reckon.h \
	    ${LAYERED}/lib/liblayered.a \
	    ${MIN}/lib/libmin.a \
	    Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -c ../src/reckon_parser.cc

reckon:		\
	    ../src/reckon.cc \
	    reckon_parser.o \
	    ../include/reckon.h \
	    ${LAYERED}/lib/liblayered.a \
	    ${MIN}/lib/libmin.a \
	    Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include \
	    -I ${LAYERED}/include -I ${MIN}/include \
	    -o $@ \
	    ../src/reckon.cc \
	    reckon_parser.o \
	    ${LAYERED}/lib/liblayered.a \
	    ${MIN}/lib/libmin.a

reckon_lexeme_test.out:	\
	    reckon reckon_lexeme_test.in
	rm -f $*.out
	${RUNENV} reckon --lexeme-test < $*.in > $*.out

reckon_bracketed_test.out:	\
	    reckon reckon_bracketed_test.in
	rm -f $*.out
	${RUNENV} reckon < $*.in > $*.out

reckon_snippet_test.out:	\
	    reckon reckon_snippet_test.in
	rm -f $*.out
	${RUNENV} reckon < $*.in > $*.out

clean:
	rm -f ${TESTS} ${TESTS:=.out} reckon

