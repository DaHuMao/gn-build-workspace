source ./build_script/tool.sh
RunAndLog 'source ./build_script/env_config.sh'
RunAndLog 'gn gen out'
RunAndLog 'ninja -C out tutorial'
./out/tutorial


