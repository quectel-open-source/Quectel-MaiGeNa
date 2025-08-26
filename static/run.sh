# 针对可执行文件进行配置
ROOT_PATH=$(cd "$(dirname "$0")/../";pwd)
BIN_PATH=$ROOT_PATH/bin/QuecVisionHJ
echo "ROOT_PATH:$ROOT_PATH"
echo "BIN_PATH:$BIN_PATH"
if [ ! -f "$BIN_PATH" ]; then
    echo "BIN_PATH:$BIN_PATH not exist"
    exit 1
fi
patchelf --set-rpath '$ORIGIN:$ORIGIN/lib:$ORIGIN/../lib' $BIN_PATH
chmod +x $BIN_PATH
export ADSP_LIBRARY_PATH=$ROOT_PATH/../lib;
export LD_LIBRARY_PATH=${ROOT_PATH}/../lib

exec $BIN_PATH -platform xcb $1 $2