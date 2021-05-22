#!/bin/bash
filename=mixtest.tcl
mobname=marged_20210518
nodenum=200

cp $filename $filename.bc &&
mobstart=11
mobend=20

#シナリオファイルのパラメータ書き換え
#ノード数の設定
sed -i -e "s/NODE_NUM_ .*/NODE_NUM_ $nodenum/g" $filename


for ((i = $mobstart ; i <= $mobend; i++))
do
  #ノードモビリティファイルの番号書き換え
  sed -i -e "s/\\\"mob\\/$mobname\_.*\\\"/\\\"mob\\/$mobname\_$i\\\"/g" $filename
  mkdir $mobname\_$i &&
  #実験モード（０，１，２，３）を順に実行
  for((g = 1 ; g <= 3; g++))
  do
    #実験モードを書き換え
    sed -i -e "s/SIMMODE_ ./SIMMODE_ $g/g" $filename
    starttime=`date "+%Y%m%d_%H%M%S"`
    #nsを実行
    ns $filename &&
    #実験結果を別ディレクトリにコピー
    mkdir $mobname\_$i/$g &&
    cp packetTraceR.csv $mobname\_$i/$g &&
    cp packetTrace.csv $mobname\_$i/$g &&
    cp res.tr $mobname\_$i/$g &&
    touch $mobname\_$i/runtime.txt &&
    echo $g:start:$starttime >> $mobname\_$i/runtime.txt
    echo $g:end  :$endtime >> $mobname\_$i/runtime.txt
  done
done

