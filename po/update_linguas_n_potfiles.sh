#!/bin/sh

ls *.po | sed 's/\.po$//' > LINGUAS
find ../ -name *.[ch] | xargs grep -l "_(\"" > POTFILES