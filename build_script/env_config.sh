source ./build_script/tool.sh
which gclient || { echo no gclient && exit 1; }
RunAndLog 'gclient sync --verbose --nohook'

