#!/bin/bash
SITE=https://pdp-10.trailing-edge.com/k20v7b/01
for F in $( cat ../klad20-a10s.list ) ; do
    echo $F : https://pdp-10.trailing-edge.com/k20v7b/01/${F}.html
    lynx -width 999999 -dump $SITE/$F.html | \
	awk -e '/^;/ {flag=1} flag; /^;EOF/ {flag=0}' > $F
done
