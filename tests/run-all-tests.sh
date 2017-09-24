interpreter=../bin/element_d

for file in $(ls *.element)
do
	echo tests from: $file
	$interpreter --test $file
done
