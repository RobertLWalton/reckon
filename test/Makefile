# Reckon Makefile
#
# File:		Makefile
# Author:	Bob Walton (walton@acm.org)
# Date:		Mon May 20 04:18:32 EDT 2024
#
# The authors have placed this program in the public
# domain; they make no warranty and accept no liability
# for this program.

# The following locates the MIN and LAYERED source code.
#
MIN = ~/min
MEX = ~/mex
LAYERED = ~/layered

# The following turns on MIN debugging.
#
GFLAGS = -DMIN_PROTECT=1
WFLAGS = -Wall -Wno-char-subscripts

.SUFFIXES:
.SUFFIXES: .out .test .diff .html

DIFFOPT ?= -u

# O3 is needed to inline functions.
#
OPTIMIZE = -O3

PARSER_TESTS = reckon_lexeme_test \
	       reckon_overview_test \
	       reckon_line_test \
	       reckon_data_test \
	       reckon_bracketed_test \
	       reckon_typed_list_test \
	       reckon_prefix_test \
	       reckon_expression_test \
	       reckon_print_test

HTML_TESTS = reckon_overview_test

.SECONDARY:	${PARSER_TESTS:=.out}

all:		diff html

diff:		${PARSER_TESTS:=.diff}

out:		${PARSER_TESTS:=.out}

test:		${PARSER_TESTS:=.test}

html:		${HTML_TESTS:=.html}

reckon_bracketed_test.out:	\
    TFLAGS = --subexpression-parse --parse-detail
reckon_data_test.out:	\
    TFLAGS = --subexpression-parse
reckon_expression_test.out:	\
    TFLAGS = --subexpression-parse --parse-detail
reckon_line_test.out:	\
    TFLAGS = --subexpression-parse
reckon_overview_test.out:	\
    TFLAGS = --subexpression-parse
reckon_overview_test.html:	\
    TFLAGS = --subexpression-parse
reckon_prefix_test.out:	\
    TFLAGS = --subexpression-parse
reckon_typed_list_test.out:	\
    TFLAGS = --subexpression-parse
reckon_print_test.out:	\
    TFLAGS = --output-parse


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

%.diff:	%.out Makefile
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
	    < $*.out | diff ${DIFFOPT} $*.test -

reckon_parser.o:            \
	    ../src/reckon_parser.cc \
	    ../include/reckon.h \
	    ${LAYERED}/lib/liblayered.a \
	    ${MEX}/lib/libmex.a \
	    ${MIN}/lib/libmin.a \
	    Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include \
	    -I ${LAYERED}/include \
	    -I ${MEX}/include \
	    -I ${MIN}/include \
	    -c ../src/reckon_parser.cc

reckon_compiler.o:            \
	    ../src/reckon_compiler.cc \
	    ../include/reckon.h \
	    ${LAYERED}/lib/liblayered.a \
	    ${MEX}/lib/libmex.a \
	    ${MIN}/lib/libmin.a \
	    Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include \
	    -I ${LAYERED}/include \
	    -I ${MEX}/include \
	    -I ${MIN}/include \
	    -c ../src/reckon_compiler.cc

reckon:		\
	    ../src/reckon.cc \
	    reckon_parser.o \
	    reckon_compiler.o \
	    ../include/reckon.h \
	    ${LAYERED}/lib/liblayered.a \
	    ${MEX}/lib/libmex.a \
	    ${MIN}/lib/libmin.a \
	    Makefile
	g++ -g ${GFLAGS} ${WFLAGS} \
	    -I ../include \
	    -I ${LAYERED}/include \
	    -I ${MEX}/include \
	    -I ${MIN}/include \
	    -o $@ \
	    ../src/reckon.cc \
	    reckon_parser.o \
	    reckon_compiler.o \
	    ${LAYERED}/lib/liblayered.a \
	    ${MEX}/lib/libmex.a \
	    ${MIN}/lib/libmin.a

%.html:	%.in reckon
	rm -f $*.html
	${RUNENV} ./reckon ${TFLAGS} --output-html \
	          < $*.in > $*.html

reckon_lexeme_test.out:	\
	    reckon reckon_lexeme_test.in
	rm -f $*.out
	${RUNENV} ./reckon --lexeme-test < $*.in > $*.out

%.out:	%.in reckon
	rm -f $*.out
	${RUNENV} ./reckon ${TFLAGS} < $*.in > $*.out

clean:
	rm -f ${PARSER_TESTS:=.out} reckon *.o

