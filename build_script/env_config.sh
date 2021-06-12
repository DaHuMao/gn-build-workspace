source ./build_script/tool.sh
RunAndLog 'git submodule update --init'
export PATH="./depot_tools:$PATH"
export DEPOT_TOOLS_UPDATE=0
cd depot_tools
RunAndLog 'eval ./gclient 1>/dev/null'
cd ../
RunAndLog 'gclient sync --verbose --nohook'

