#!/bin/bash
if [ ! x$# = x2 ];then

	echo usage: param1: C filer para2 output header file
	exit 0
fi
headfile=$2
headname=`basename $2`
headname_1=${headname%.*}
headdef=_${headname//-/_}
headdef=_${headdef//./_}
headdef1=`echo $headdef | tr 'a-z' 'A-Z'`

echo "#ifndef $headdef1" >$headfile
echo "#define $headdef1" >>$headfile
echo " " >>$headfile
echo " " >>$headfile

filename=`basename $1`

while read LINE
do
        if echo $LINE|grep '.clk_register_fixed_rate' >/dev/null;then
        string1=${LINE#*(}
        string2=${string1#*,}
        string3=${string2%%,*}
        string4=${string3#*\"}
        string5=${string4%%\"*}

        define_name=`echo $string5 | tr 'a-z' 'A-Z'`
        define_name="${define_name}_CLK"
        echo "#define $define_name" '"'$string5'"' >>$headfile
        fi
        if echo $LINE|grep '.clk_register_fixed_factor' >/dev/null;then
        string1=${LINE#*(}
        string2=${string1#*,}
	string3=${string2%%,*}
    	string4=${string3#*\"}
        string5=${string4%%\"*}

        define_name=`echo $string5 | tr 'a-z' 'A-Z'`
        define_name="${define_name}_CLK"
        echo "#define $define_name" '"'$string5'"' >>$headfile
        fi
done < $1

while read LINE
do
	if echo $LINE|grep 'sunxi_clk_factor_'|grep 'get_factor'|grep 'calc_rate' >/dev/null;then
	string1=${LINE#*\"}
	string2=${string1%%\"*}
	define_name=`echo $string2 | tr 'a-z' 'A-Z'`
	define_name="${define_name}_CLK"
	echo "#define $define_name" '"'$string2'"' >>$headfile
	fi
	if echo $LINE|grep '_parents'|grep 'ARRAY_SIZE('|grep 'sunxi_clk_periph' >/dev/null;then
        string1=${LINE#*\"}
        string2=${string1%%\"*}
        define_name=`echo $string2 | tr 'a-z' 'A-Z'`
        define_name="${define_name}_CLK"
        echo "#define $define_name" '"'$string2'"' >>$headfile
	fi
done < $1
echo " " >>$headfile
echo " " >>$headfile

echo '#endif' >>$headfile
#echo ${filename%%.*}".o"
