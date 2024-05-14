# pico-disp-ident

## これは何？

- RP2040 搭載ボードと PicoDVI (https://github.com/Wren6991/PicoDVI) を使って、DVI ディスプレイに番号入りのテストパターン表示をさせるツールです。
- PicoDVI の feat/x8scale ブランチ (https://github.com/homelith/PicoDVI/tree/feat/x8scale) 版を用いることで、80x60 画像を VGA 640x480 ディスプレイに引き延ばして表示します。

## ハードウェア構築

- 記載中

## ソフトウェアビルド

- 環境準備
  + Linux ベースの OS 各種上でコンパイル可能と想定 (x86_64 ネイティブの ubuntu 22.04 LTS と WSL2 上の ubuntu 22.04 LTS で試験済)
  + パッケージインストール : `sudo apt install git gcc make cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential golang`

- uf2 バイナリをコンパイルする手順
  + 下記手順でコンパイル後、`sw/build/app/pico-disp-ident.uf2` が得られます

```
$ git clone --recursive git@github.com:homelith/pico-disp-ident.git
$ cd pico-disp-ident/sw
$ make
```

## デバイスを動作させる手順

- Raspberry Pi Pico への uf2 ファイルダウンロード
  + BOOTSEL ボタンを押しながら Raspi Pico の電源を投入することで、USB ポートがマスストレージデバイスとして認識されます
  + `pico-disp-ident.uf2` ファイルを該当ストレージの直下に置くことで書き込みとセルフリセットが行われます

- ボタンを押下すると表示画像と対応する 7seg の表示番号が切り替わります
