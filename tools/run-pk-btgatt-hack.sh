#!/bin/bash

#export PATH=`pwd`:$PATH
exe_name=pk-btgatt-hack

if [ -f $exe_name ]
then
  exe_name=./$exe_name
  echo "setting exe name: $exe_name"
  sleep 0.5
else
  echo "ok, using $exe_name from the path..."
fi

echo $exe_name $*

while $exe_name  $* ; do
   echo "***!!! Relaunching..."
done

SpawnStopToken="spawn-token.tmp"
echo "executable finished; Respawning unless file:"
echo "                 ***** $SpawnStopToken ***** "
echo "                                exists in `pwd` ."	
sleep 2
echo "....patience..."
sleep 5
echo " ok, respawning should happen."
sleep 2

if [ ! -f $SpawnStopToken ]
then
  gnome-terminal -e "./run-pk-btgatt-hack.sh $* "
fi

echo "done with runner instance"

