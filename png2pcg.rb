#
# PNGファイルをX1用のPCGデータに変換する
# 2021-01-21 programed by Meister
#
# These codes are licensed under CC0.
# http://creativecommons.org/publicdomain/zero/1.0/deed.ja
#

# 画像のディザリングやリサイズ等をしないので
# あらかじめ128x128 8色にしておくこと

# データフォーマット
# 
# 文字コード 0x00・1ライン目の青(1byte)
# 文字コード 0x00・1ライン目の赤(1byte)
# 文字コード 0x00・1ライン目の緑(1byte)
# 文字コード 0x00・2ライン目の青(1byte)
#      ・
#      ・
#      ・
# 文字コード 0x00・8ライン目の緑(1byte)
# 文字コード 0x01・1ライン目の青(1byte)
#      ・
#      ・
#      ・
# 文字コード 0xff・8ライン目の緑(1byte)
# 全6144byte


require 'chunky_png'
require 'pp'


# 垂直型のビットマップから指定プレーンのデータを抜き出す
# plane : 1(青)，2(緑)，3(赤)
def get_plane(image_row, plane)
    mask = 0xff << (8 * plane)
    threshold = 0x7f << (8 * plane)

    image_row.map {|pixel|
        ((pixel & mask) > threshold) ? 1 : 0
    }.each_slice(8).map {|dat|
        dat.inject(0) {|result, item| result * 2 + item}
    }
end


ARGV.each {|filename|
    print "#{filename} -> "

    image = ChunkyPNG::Image.from_file(filename)

    # 8ラインごとに処理する
    pcgdat = image.height.times.each_slice(8).map {|yy|
        yy.map {|y|
            plane_b = get_plane(image.row(y), 1)
            plane_g = get_plane(image.row(y), 2)
            plane_r = get_plane(image.row(y), 3)
        
            # 並び変える
            plane_b.zip(plane_r, plane_g).map {|dat| dat.pack('CCC')}
        }.transpose.map(&:join)
    }.flatten.join

    outfile = filename.sub(/\.png$/i,'.PCG')
    puts(outfile)
    open(outfile, 'wb') {|ofs| ofs.write(pcgdat)}
}
