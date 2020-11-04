#!/bin/bash

dtsi_file=$1
if [ -z "$dtsi_file" ] || [ ! -f $dtsi_file ]; then
	echo "dts file: $dtsi_file not found!"
fi

# step1: prepare some tmp files
# decompose the original dtsi to five parts,
# then compose into new dtsi
dts_path=$(dirname $dtsi_file)
dts_name=$(basename -s .dtsi $dtsi_file)
dtsi_bak_file="$dts_path/${dts_name}_bak.dtsi"
dtsi_start_file="$dts_path/${dts_name}_start.dtsi"
dtsi_sdc2_file="$dts_path/${dts_name}_sdc2.dtsi"
dtsi_mid_file="$dts_path/${dts_name}_mid.dtsi"
dtsi_sdc0_file="$dts_path/${dts_name}_sdc0.dtsi"
dtsi_end_file="$dts_path/${dts_name}_end.dtsi"

# step2: decompose the old_dtsi_file=start:sdc2:mid:sdc0:end
# search the sdc2 head
sdc2_start=$(sed -n '/^\s*sdc2:/=' $dtsi_file)

# search the sdc0 head
sdc0_start=$(sed -n '/^\s*sdc0:/=' $dtsi_file)

# sdc2_start < sdc0_start normally in dtsi
if [ "$sdc2_start" -gt "$sdc0_start" ]; then
	echo "sdc2 is after sdc0, may be something wrong"
	exit 0
fi

cp $dtsi_file $dtsi_bak_file
cp $dtsi_file $dtsi_end_file

sed       '/^\s*sdc2:/Q'    $dtsi_end_file > $dtsi_start_file
sed -i -n '/^\s*sdc2:/,$ p' $dtsi_end_file

sed -n    '1,/^\s*};/p'     $dtsi_end_file > $dtsi_sdc2_file
sed -i    '1,/^\s*};/d'     $dtsi_end_file

sed       '/^\s*sdc0:/Q'    $dtsi_end_file > $dtsi_mid_file
sed -i -n '/^\s*sdc0:/,$ p' $dtsi_end_file

sed -n    '1,/^\s*};/p'     $dtsi_end_file > $dtsi_sdc0_file
sed -i    '1,/^\s*};/d'     $dtsi_end_file

# step3: compose the new_dtsi_file=start:sdc0:mid:sdc2:end
cat $dtsi_start_file $dtsi_sdc0_file $dtsi_mid_file $dtsi_sdc2_file $dtsi_end_file > $dtsi_file

# step4:clean some tmp files
rm -rf $dtsi_start_file
rm -rf $dtsi_sdc2_file
rm -rf $dtsi_mid_file
rm -rf $dtsi_sdc0_file
rm -rf $dtsi_end_file
