#!/bin/bash

# 2MB
#./mkspiffs -c ./www/ -p 256 -s 1507328 ./www.bin

# 1MB
#./spiffsgen.py --page-size 256 --block-size 4096 --obj-name-len 32 --meta-len 4 --use-magic --use-magic-len 0x70000 ./www www.bin
KCONFIG="sdkconfig"

PAGE_SIZE=$(cat ${KCONFIG} | grep CONFIG_SPIFFS_PAGE_SIZE | tr -d 'CONFIG_SPIFFS_PAGE_SIZE=')
OBJ_NAME_LEN=$(cat ${KCONFIG} | grep CONFIG_SPIFFS_OBJ_NAME_LEN | tr -d 'CONFIG_SPIFFS_OBJ_NAME_LEN=')
USE_MAGIC=$(cat ${KCONFIG} | grep 'CONFIG_SPIFFS_USE_MAGIC=' | tr -d 'CONFIG_SPIFFS_USE_MAGIC=')
USE_MAGIC_LEN=$(cat ${KCONFIG} | grep 'CONFIG_SPIFFS_USE_MAGIC_LENGTH=' | tr -d 'CONFIG_SPIFFS_USE_MAGIC_LENGTH=')
META_LENGTH=$(cat ${KCONFIG} | grep CONFIG_SPIFFS_META_LENGTH | tr -d 'CONFIG_SPIFFS_META_LENGTH=')

echo "PAGE_SIZE=$PAGE_SIZE"
echo "OBJ_NAME_LEN=$OBJ_NAME_LEN"
echo "USE_MAGIC=$USE_MAGIC"
echo "USE_MAGIC_LEN=$USE_MAGIC_LEN"
echo "META_LENGTH=$META_LENGTH"

OPTION="--block-size 4096 --page-size ${PAGE_SIZE} --obj-name-len ${OBJ_NAME_LEN} --meta-len ${META_LENGTH} "

if [ $USE_MAGIC = "y" ]
then
  OPTION="$OPTION --use-magic "
fi

if [ $USE_MAGIC_LEN = "y" ]
then
  OPTION="$OPTION --use-magic-len"
fi

echo "options: $OPTION"

SIZE=0x70000

./spiffsgen.py $OPTION $SIZE ./www www.bin

echo "done"
