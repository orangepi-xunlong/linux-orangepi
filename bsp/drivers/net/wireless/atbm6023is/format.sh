#!/bin/bash

find -type f | grep -v format.sh | xargs chmod 644

find -type f | grep -v format.sh | xargs sed -i 's|^\(\s*\)#pragma|\1//#pragma|g'

find -type f | grep -v format.sh | xargs sed -i \
-e 's|||g' \
-e 's|\s\+$||g' \
-e 's|){$|) {|g' \
-e 's|^\(\s*\)if(|\1if (|g' \
-e 's|\(\S\)\s\+) {$|\1) {|g' \
-e 's|^\(\s*\)if (\s\+|\1if (|g' \
-e 's|\(\S\)\s\+)$|\1)|g'
