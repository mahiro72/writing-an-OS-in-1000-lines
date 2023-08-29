# カーネルのエントリーポイントはboot関数
ENTRY(boot)

SECTIONS {
	# ページアドレス
	# . 現在のアドレス
	. = 0x80200000;

	# カーネルが利用するアドレスの先頭を設定
	__kernel_base = .;
	
	# .text.bootセクションを先頭に配置
	# .text: コード領域
	.text : {
		KEEP(*(.text.boot));
		# 全ファイル(*)中の.text, .textから始まる名前のセクションをそこに配置する
		*(.text .text.*);
	}

	# 定数データ領域	
	# ALIGN(4) 4バイト領域になるようにアドレスを調整
	.rodata : ALIGN(4) {
		*(.rodata .rodata.*);	
	}
	
	# 読み書き可能データ領域 
	.data : ALIGN(4) {
		*(.data .data.*);
	}

	# 読み書き可能データ領域, 未初期化の静的データ領域を指す
	# Block Started by Symbol または Block Started by Storageの略
	.bss : ALIGN(4) {
		# __bssシンボルに現在のアドレスを割り当てる. シンボルは関数や静的変数を表すもの
		# C言語で extern char シンボル名 で定義したシンボルを参照できる
		__bss = .;
		*(.bss .bss.* .sbss .sbss.*);
		__bss_end = .;	
	}
	
	. = ALIGN(4);
	. += 128 * 1024;  /* 128kb */
	__stack_top = .;

	# 4KB領域に配置されるようになる
	# リンカスクリプト上に定義することで、カーネルの静的領域と被らないようにリンカが位置を決定してくれる
	. = ALIGN(4096);
	__free_ram = .;
	. += 64 * 1024 * 1024; /* 64MB */
	__free_ram_end = .; 
}


