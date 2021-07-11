source ./build_script/tool.sh
RunAndLog 'source ./build_script/env_config.sh'
RunAndLog 'gn gen out --args='is_debug=true' --ide=xcode'
RunAndLog 'ninja -C out tutorial'
./out/tutorial


