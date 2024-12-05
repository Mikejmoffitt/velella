# Velella・ヴェレラ

Velella is a pixel art conversion tool. PNG files are read, and the following is emitted:

- Binary pixel data
- Palette data
- Metadata listing

ヴェレラはドット絵変換ソフトです。　PNGファイロを読み込むと、下記のファイルが出ます。
-バイナリードットデータ　（CHRと言う）
-パレットデータ　（変換された色）
-入っている絵の情報　（コード番号やデータ相対変位とか）

# OS・オペレーティングシステム

Velella was created on and for Linux, but as it has no esoteric dependencies it ought to build as-is on WSL or Mac OS, and can probably build natively for Windows with MSYS as well. You can probably build it on anything.
ヴェレラはLinuxで作られたけど、WSLでWindowsでも問題なくコンパイルできるはずです。　MacOSも同じような話になると思います。　特別のライブラリー使ってないんです。

Here are the build and install instructions:

	`$ make && sudo make install`

# Disclaimer・免責条項

Instead of writing new conversion tools for different formats and filetypes, I'm making an effort to roll them into a single tool that I can maintain. I add things to this as I need them for a project, and don't make claims about this tool's preparedness to handle any use case. I can't even guarantee it's very good, just that it's been adequate for me to complete tasks with it!

今までそのばその場新しいツールを書いて使っていたんですが、なるべく前書いたコード何回使えるように一つのソフトに入れておいています。　自分のために書いたので、もし使えばいいんですけど、全存在している形式追加つもり訳ではありません。
ていうか、必要なファイル形式を場合ずつ追加しているで、もし今のツールが足りなかったら仕方がないです。　優しく聞いたらできるかもしれません。　自分でして、PR作ったら一番いいです。

その上、当たり前と思うんですが、日本語は母語ではないので、何かが不明だったら申し訳ございません。　自動的な翻訳より自分で頑張って下手くそ日本語書くのはいいと感じますけど。

# Usage・使い方

A conversion script, suspiciously similar to an INI file, is provided to the program to configure the conversion parameters and indicate source file(s). Asset data is tagged with a symbol name, and once the source is indicated it will be added. Once all input files have been processed, the relevant output files are generated.

ヴェレラはスクリップトを読み込んで、行ごとに設定調整されたりファイル変換したりします。　追加した絵データは名前をつけて、設定によって処理させて、バイナリーデーターとして出ます。

## Script Commands

Variables modified by commands will persist through different symbols and files, so it is not necessary to repeat yourself. Numerical options always support hexidecimal input by way of the C `0x` prefix.

変数を設定すると、勝手に変わらないので、変化したくないことをそのまましても大丈夫です　（例えば、もし全絵は同じサイズだったら一回さえ言ったらオッケーです）。
番号の変数は、１６進数も１０進数も使えます。

### `out`
Specifies the base output filename. There is no default value, so it must be set.
There should be no filename extension, as Velella will append ones for the different filetypes.
土台出力ファイル名。　（初期設定：無）
ファイル確証し書かないで下さい。　このファイルネームに基づいてベレラは自動的に多くのファイルを作るんです。

For example, for the line
例えば、この例行は

    `out = sample`

you get
このファイルになります。

- `sample.chr`
- `sample.pal`
- `sample.inc 


### `format`
Specifies the output data format. There is no default value, so it must be set.
出力ドットファイル形式です。　（初期設定：無）

Supported values include:
使える設定：
- `bg038`
- `sp013`
- `direct`


### `palette`
Specifies the palette data format. Thre is no default value, so it must be set.
出力色データーファイル形式です。　（初期設定：無）
Supported values include:
使える設定：
- `atlus`
- `x68000`
- `md`
- `cps`


### `depth`
The indexed color depth in bits per pixel. 4 and 8 are common values, and which values are supported vary based on the chosen data format.
The default value is 4.
絵のドットのビット数です。　（初期設定：４）
４は一般的です。　出力ファイル形式によって使える設定が変わります。


### `code`
Starting code value to increment from in the metadata.
The default value is 0.
絵のコード番号の初期設定です。　（初期設定：０）
この情報はインクルードファイルに出ます。


### `x` and `y`
Size of a frame within the image. If left at 0, the whole image is processed in one pass, as a single "frame". For a background, or a tileset, having frames may not be very useful, so leaving it at 0 is fine.
The default value is 0.
絵の中のフレーム解像度です。　（初期設定：０）
０にすると、一つのフレームとして絵を全ては同時処理されます。　背景の絵やタイルセットならフレームは無用かもしれないので、０にしても大丈夫です。


### `tilesize`
Dictates the size of a tile, which is the minimum building block for many graphics systems. If the tilesize is less than that of a frame, this affects how the pixel data is traversed. The meaning of this varies based on the `format` argument so I will not go into detail here.
The default value is 16.


### `angle`
Rotation to apply to frames. Only 90-degree increments are supported; 270 is common for portrait orientation games.
Default value is 0.
絵の回転です。　（初期設定：０）
直角しか使えません。 ２７０度は縦シューティングゲームには一般的です。


### `[symbol]`
In the [brackets], a symbolic name to assign to an image is defined. You can use this to include multiple images in one conversion pass, and pack them in the same binary blob.
変換するへの名前を付けます。　PNGファイルずつsymbolを使って下さい。


### `src`
Source image filename for the current symbol. When this is specified, the file read is initiated. You can include multiple images with the same spec by just defining a new symbol and source line.
ソース（入力）絵のファイル名です。　symbolのあとsrcを使って下さい。
srcは処理させると、ファイル変換が始まります。
