#!/bin/bash

i=1
while [ -a ./tests/example"$i".img ]
do
	echo $i
	./sim_main ./tests/example"$i".img > ./logs/out$i.log
	diff -y --suppress-common-lines ./logs/out$i.log ./ref_results/example"$i"_output
	(( i++ ))
done
