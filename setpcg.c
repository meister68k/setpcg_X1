//
// X1 with LSX-Dodgers用 PCG定義コマンド
//
// 2021-01-21 programed by Meister
//
// These codes are licensed under CC0.
// http://creativecommons.org/publicdomain/zero/1.0/deed.ja

// データフォーマット
// 
// 文字コード 0x00・1ライン目の青(1byte)
// 文字コード 0x00・1ライン目の赤(1byte)
// 文字コード 0x00・1ライン目の緑(1byte)
// 文字コード 0x00・2ライン目の青(1byte)
//      ・
//      ・
//      ・
// 文字コード 0x00・8ライン目の緑(1byte)
// 文字コード 0x01・1ライン目の青(1byte)
//      ・
//      ・
//      ・
// 文字コード 0xff・8ライン目の緑(1byte)
// 全6144byte

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 24倍速PCG定義ルーチン
// 参考文献:試験に出るX1,祝 一平，日本ソフトバンク,1987
//
// 引数:
//   *p : PCGデータ 8文字分 (合計192バイト)
// 
// ・VSYNCを監視
// ・タイミングが来たら1ライン分のPCGデータ(青・赤・緑)を順次OUTI
// ・次のスキャンラインにソフトウェアディレイでタイミングを合わせる
// ということをやっている
//
// このルーチンはVRAM下部8行(7行+半端1行)を使用して一度に8文字登録する
//
void setpcg_main(uint8_t* p) __z88dk_fastcall __naked
{
    // z88dkの仕様で HLレジスタ = 引数*p になっている

    #asm
    ld      b, 0x16             ; PCG I/O BLUE  = 0x15 + 1 (because OUTI ...)
    ld      d, 0x17             ; PCG I/O RED   = 0x16 + 1
    ld      e, 0x18             ; PCG I/O GREEN = 0x17 + 1
    ld      c, 0                ; PCG I/O 0x1?00
    ld      a, 64               ; line counter 8 * 8 characters

    // wait vsync
    ex      af, af
    exx
    di
    ld      bc, 0x1a01          ; V-DISP port (bit7)
vdisp0:
    in      a, (c)
    jp      p, vdisp0
vdisp1:
    in      a, (c)
    jp      m, vdisp1
    exx
    ex      af, af

setp:
    outi                        ; BLUE
    ld      b, d
    outi                        ; RED
    ld      b, e
    outi                        ; GREEN
    ld      b, 0x16             ; PCG I/O BLUE  = 0x15 + 1

    ex      af, af
    ld      a, 0x0b
dly:
    dec     a
    jp      nz, dly
    ex      af, af

    inc     c
    dec     a
    jp      nz, setp            ; loop cycle = 250 clock

    ei
    ret
    #endasm

    // z88dkの仕様で__naked属性の関数はretまでインラインアセンブラで書かなければならない
}


#if 0
// 3倍速バージョン
//
// 引数：
//   *p : PCGデータ 1文字分 (合計24バイト)
//
void setpcg_main_3x(uint8_t* p) __z88dk_fastcall __naked
{
    // HL = *p

    #asm
    ld      b, 0x16             ; PCG I/O BLUE  = 0x15 + 1 (because OUTI ...)
    ld      d, 0x17             ; PCG I/O RED   = 0x16 + 1
    ld      e, 0x18             ; PCG I/O GREEN = 0x17 + 1
    ld      c, 0                ; PCG I/O 0x1?00
    ld      a, 8                ; line counter 8 lines

    // wait vsync
    ex      af, af
    exx
    di
    ld      bc, 0x1a01          ; V-DISP port (bit7)
vdisp0:
    in      a, (c)
    jp      p, vdisp0
vdisp1:
    in      a, (c)
    jp      m, vdisp1
    exx
    ex      af, af

setp:
    outi                        ; BLUE
    ld      b, d
    outi                        ; RED
    ld      b, e
    outi                        ; GREEN
    ld      b, 0x16             ; PCG I/O BLUE  = 0x15 + 1

    ex      af, af
    ld      a, 0x0b
dly:
    dec     a
    jp      nz, dly
    ex      af, af

    inc     c
    dec     a
    jp      nz, setp            ; loop cycle = 250 clock

    ei
    ret
    #endasm
}

// 等速バージョン
//
//
// 引数：
//   *p : 出力ポート+PCGデータ 1プレーン・1文字分 (合計9バイト)
//
// データの先頭で出力ポート(0x15 BLUE～0x17 GREEN)を指定する
void setpcg_slow(uint8_t* p) __z88dk_fastcall __naked
{
    #asm
    ld      b, (hl)
    inc     hl
    ld      c, 0
    ld      e, 8

    // wait vsync
    exx
    di
    ld      bc, 0x1a01          ; V-DISP port (bit7)
vdisp0:
    in      a, (c)
    jp      p, vdisp0
vdisp1:
    in      a, (c)
    jp      m, vdisp1
    exx

setp:
    ld      a, (hl)
    out     (c), a
    inc     hl
    inc     bc
    nop
    inc     hl
    dec     hl
    ld      a, 0x0d
dly:
    dec     a
    jp      nz, dly
    dec     e
    jp      nz, setp

    ei
    ret
    #endasm
}
#endif


// 全文字種にPCG定義
void setpcg(uint8_t *pcgdata)
{
    uint8_t* pat = pcgdata;
    uint8_t c = 0;

    // 8文字ごとにループ
    do {
        // 余白VRAMに定義対象の文字コードを書き込む
        // 最終行(26行目)は48文字しかないので，そこに長さを揃える
        for(uint8_t x = 0; x < 48; x++) {
            outp(0x3000 + 80 * 18 + x, c    );
            outp(0x3000 + 80 * 19 + x, c + 1);
            outp(0x3000 + 80 * 20 + x, c + 2);
            outp(0x3000 + 80 * 21 + x, c + 3);
            outp(0x3000 + 80 * 22 + x, c + 4);
            outp(0x3000 + 80 * 23 + x, c + 5);
            outp(0x3000 + 80 * 24 + x, c + 6);
            outp(0x3000 + 80 * 25 + x, c + 7);
        }
        setpcg_main(pat);
        pat += 8 * 3 * 8;           // 8文字分データポインタを進める
        c += 8;
    } while(c != 0);

    return;
}


// CRTCの表示行数を変更する
void set_crtc_lines(uint8_t lines)
{
    outp(0x1800, 6);                    // CRTCレジスタ選択
    outp(0x1801, lines);                // CRTCデータ
}


// VRAM消去
// 指定アドレス以降のテキストとアトリビュートを消去する
// アドレスをアトリビュート側で指定する
void cls_vram(uint16_t st_addr, uint8_t attr, uint8_t chr)
{
    for(uint16_t addr = st_addr; addr < 0x2800; addr++) {
        outp(addr, attr);
        outp(addr + 0x1000, chr);
    }
}


// 使用法を表示して終了
void usage_and_abort()
{
    fputs(
        "Usage: setpcg [-v|-w] [filename]\n"
        "Set PCG data from a file. (only for Sharp X1)\n"
        "  -v    Preview all characters.\n"
        "  -w    Preview all characters.(wide mode)\n\n"
        "2021 programed by Meister (http://github.com/meister68k/setpcg)\n"
        "This program is licensed under CC0.\n"
        "http://creativecommons.org/publicdomain/zero/1.0/deed.ja",
        stderr
    );

    abort();
}


int main(int argc, char* argv[])
{
    // 全データのサイズ
    // 1byte/ライン・プレーン × 8ライン/文字 × 3プレーン × 256文字
    // == 6144byte
    const uint16_t datasize = 8 * 3 * 256;

    char* filename = NULL;              // データファイル名
    uint8_t opt_preview = 0;            // プレビューモード

    for(uint8_t i = 1; i < argc; i++) {
        if(*(argv[i]) == '-') {
            // オプションの確認
            // CP/Mはオプションを勝手に大文字に変換，LSX-Dodgersはそのままなので注意
            if((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "-V") == 0)) {
                opt_preview = 1;        // プレビューあり
            } else if((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "-W") == 0)) {
                opt_preview = 2;        // 倍角プレビューあり
            } else usage_and_abort();
        } else if(!filename) {
            filename = argv[i];
        } else usage_and_abort();
    }

    if(!filename && !opt_preview) usage_and_abort();

    // ファイルからPCG定義
    if(filename) {
        FILE* fp;
        if((fp = fopen(filename, "rb")) == NULL) {
            fprintf(stderr, "%s not found.", filename);
            abort();
        }

        uint8_t* pcgdata = (uint8_t*) malloc(datasize);
        fread(pcgdata, 1, datasize, fp);

        // 24倍速PCG定義の準備
        set_crtc_lines(18);             // 表示行数を18行に減らす
        // PCG定義用に余白VRAMのアトリビュートを準備する
        cls_vram(0x2000 + 80 * 18, 0x20, 0);

        setpcg(pcgdata);                // 全文字種にPCG定義

        // 後片付け
        // OS(LSX-Dodgeres)が最終行を消してくれない
        cls_vram(0x2000 + 80 * 18, 7, ' ');
        set_crtc_lines(25);             // 表示行数を戻す
        printf("\033E");                // CLS
    }

    // 確認用に文字全種をPCGで描く
    if(opt_preview) {
        uint8_t c = 0;
        for(uint8_t y = 0; y < 16; y++) {
            uint16_t addr = 0x2000 + y * 80;
            for(uint8_t x = 0; x < 16; x++) {
                if(opt_preview != 2) {
                    outp(addr, 0x27);
                    outp(addr + 0x1000, c);
                } else {
                    // 水平2倍モード
                    outp(addr, 0xa7);
                    outp(addr + 0x1000, c);
                    addr++;
                    outp(addr, 0xa7);
                    outp(addr + 0x1000, c);
                }
                addr++;
                c++;
            }
        }
        printf("\033Y%c ", 0x20 + 17);  // LOCATE 0,17
    }

    return 0;
}
