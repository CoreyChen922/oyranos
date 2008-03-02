#!/bin/sh -x

# exportsymbols.sh
# ----------------
#
# Export kdbBackendFactory entry point symbols
# for each compiled backend
#
# (c) 2006 Yannick Lecaillez

func=kdbBackendFactory
toolsfuncs='ksFromXMLfile ksFromXML'

echo '
/* exported_symbols.c generated by exportsymbols.sh */

#include "kdbbackend.h"
#include "kdbLibLoader.h"

/* Make the compiler happy */
' > exported_symbols.c

for lib in "$@"
do
	if test $lib = libelektratools; then
		for toolsfunc in $toolsfuncs; do
			symbol=${lib}_LTX_${toolsfunc}
			echo "extern int $symbol;" >> exported_symbols.c
		done
	else
		symbol=libelektra_${lib}_LTX_${func}
		echo "Exporting symbols for $lib ..."
		echo "extern int $symbol;" >> exported_symbols.c
	fi
done

echo '
kdblib_symbol kdb_exported_syms[] =
{
' >> exported_symbols.c

for lib in "$@"
do
	if test $lib = libelektratools; then
		fname=libelektratools
		echo -e "\t{\"$fname\", NULL}," >> exported_symbols.c
		for toolsfunc in $toolsfuncs; do
			symbol=${fname}_LTX_${toolsfunc}
			echo -e "\t{\"$toolsfunc\", &$symbol }," >> exported_symbols.c
		done
	else
		fname="libelektra-${lib}"
		symbol=libelektra_${lib}_LTX_${func}
		echo -e "\t{\"$fname\", NULL}," >> exported_symbols.c
		echo -e "\t{\"$func\", &$symbol }," >> exported_symbols.c
	fi
	echo "" >> exported_symbols.c
done
echo -e "\t{ NULL, NULL }" >> exported_symbols.c
echo "};" >> exported_symbols.c
