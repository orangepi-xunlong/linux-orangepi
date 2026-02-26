#!/bin/bash

if [ $# != 1 ]; then
     echo Please enter the path of spectrum;
     exit 1;
fi

#spectrum_raw_data_file='/home/willlu/Desktop/rx_spectrum.txt'
spectrum_raw_data_file=$1
spectrum_file='rx_spectrum_value.txt'

if [ ! -f "$spectrum_raw_data_file" ]; then
     echo spectrum file not found.;
     exit 1;
fi

spectrum_values=$(sed '/The spectrum / !d' $spectrum_raw_data_file | awk '{print $12}')
for value in $spectrum_values
do
    echo "l(($value))/l(10)" | bc -l >> rx_spectrum.xls
done
