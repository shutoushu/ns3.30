#!/bin/sh
filename=trips.trips.xml
cp $filename $filename.bak;
sed -e 's/depart=""[0-9].00"/depart="0.00"/g' $fiename > temp.$filename;
sed -e 's/depart=""[0-9.][0-9.].00"/depart="0.00"/g' $fiename > temp.$filename;
rm temp.$filename
