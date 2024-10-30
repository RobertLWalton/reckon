# Reckon Makefile
#
# File:		Makefile
# Author:	Bob Walton (walton@acm.org)
# Date:		Tue Oct 29 05:40:09 AM EDT 2024
#
# The authors have placed this program in the public
# domain; they make no warranty and accept no liability
# for this program.

R = ../../test/reckon

.SUFFIXES:
.SUFFIXES: .rec .out .trace .mex .parse \
	   .trace_test .trace_diff \
	   .run_test .run_diff \
	   .compile_test .compile_diff \
	   .parse_test .parse_diff

DIFFOPT ?= -u

RUN_TESTS = reckon_number_constants \
	    reckon_assignments \
	    reckon_logical \
	    reckon_block_assignments \
	    reckon_loops \
	    reckon_numbers \
	    reckon_errors
PARSE_TESTS = reckon_lexemes \
	      reckon_logical_lines \
	      ${RUN_TESTS}

.SECONDARY:	${RUN_TESTS:=.trace} \
		${RUN_TESTS:=.out} \
		${RUN_TESTS:=.mex} \
		${PARSE_TESTS:=.parse}

all:		parse_diff compile_diff run_diff \
		trace_diff

parse_diff:	${PARSE_TESTS:=.parse_diff}
compile_diff:	${RUN_TESTS:=.compile_diff}
run_diff:	${RUN_TESTS:=.run_diff}
trace_diff:	${RUN_TESTS:=.trace_diff}
parse_test:	${PARSE_TESTS:=.parse_test}
compile_test:	${RUN_TESTS:=.compile_test}
run_test:	${RUN_TESTS:=.run_test}
trace_test:	${RUN_TESTS:=.trace_test}
parse:		${PARSE_TESTS:=.parse}
compile:	${RUN_TESTS:=.mex}
run:		${RUN_TESTS:=.out}
trace:		${RUN_TESTS:=.trace}

SED = sed \
    -e '/\.cc:[0-9]* assert/s//.cc:XXXX assert/' \
    -e '/\.h:[0-9]* assert/s//.h:XXXX assert/' \
    -e '/\.cc:[0-9]* desire/s//.cc:XXXX desire/' \
    -e '/\.L/s/\(\.L[A-Z][A-Z]*\)[0-9][0-9]*/\1XXX/g' \
    -e '/line [0-9]/s//line \#/g' \
    -e '/\(lines [0-9][0-9]*-\)[0-9]/s//\1\#/g' \
    -e '/lines [0-9]\([0-9]*-\#\)/s//lines \#\1/g' \
    -e '/\#[0-9]/s//\#\#/g' \
    -e '/\#[0-9]/s//\#\#/g' \
    -e '/\#[0-9]/s//\#\#/g' \
    -e '/\#[0-9]/s//\#\#/g' \
    -e '/\#[0-9]/s//\#\#/g' \
    -e '/[0-9]\#/s//\#\#/g' \
    -e '/[0-9]\#/s//\#\#/g' \
    -e '/[0-9]\#/s//\#\#/g' \
    -e '/[0-9]\#/s//\#\#/g' \
    -e '/[0-9]\#/s//\#\#/g' \
    -e '/@[0-9]/s//@@/g' \
    -e '/@[0-9]/s//@@/g' \
    -e '/@[0-9]/s//@@/g' \
    -e '/@[0-9]/s//@@/g' \
    -e '/@[0-9]/s//@@/g'

%.parse:	../examples/%.rec Makefile $R
	rm -f $*.parse
	$R --output-parse <../examples/$*.rec >$*.parse

%.mex:	../examples/%.rec Makefile $R
	rm -f $*.mex
	$R --compile <../examples/$*.rec >$*.mex

%.out:	../examples/%.rec Makefile $R
	rm -f $*.run
	$R --run <../examples/$*.rec >$*.out

%.trace:	../examples/%.rec Makefile $R
	rm -f $*.trace
	$R --trace <../examples/$*.rec >$*.trace

%.parse_test:	%.parse Makefile
	rm -f $*.parse_test
	${SED} < $*.parse > $*.parse_test

%.parse_diff:	%.parse Makefile
	@echo DIFFING $*.parse_test $*.parse
	-@${SED} \
	    < $*.parse | diff ${DIFFOPT} $*.parse_test -

%.compile_test:	%.mex Makefile
	rm -f $*.compile_test
	${SED} < $*.mex > $*.compile_test

%.compile_diff:	%.mex Makefile
	@echo DIFFING $*.compile_test $*.mex
	-@${SED} \
	    < $*.mex | diff ${DIFFOPT} $*.compile_test -

%.run_test:	%.out Makefile
	rm -f $*.run_test
	${SED} < $*.out > $*.run_test

%.run_diff:	%.out Makefile
	@echo DIFFING $*.run_test $*.out
	-@${SED} \
	    < $*.out | diff ${DIFFOPT} $*.run_test -

%.trace_test:	%.trace Makefile
	rm -f $*.trace_test
	${SED} < $*.trace > $*.trace_test

%.trace_diff:	%.trace Makefile
	@echo DIFFING $*.trace_test $*.trace
	-@${SED} \
	    < $*.trace | diff ${DIFFOPT} $*.trace_test -

clean:
	rm -f ${PARSER_TESTS:=.out} reckon *.o
