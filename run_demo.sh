source ./build_script/env_config.sh
gn gen out
ninja -C out tutorial
./out/tutorial


