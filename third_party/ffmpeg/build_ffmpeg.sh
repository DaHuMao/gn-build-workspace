build_dir="./"
if [ ! -d $build_dir ];then
  mkdir $build_dir
fi

./src/configure --prefix=$build_dir --disable-ffmpeg --disable-ffplay --disable-ffprobe \
            --enable-encoder=aac --enable-muxer=adts
make clean
make -j4
make install
