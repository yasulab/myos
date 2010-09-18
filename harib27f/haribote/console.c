/* コンソール関係 */
#include "bootpack.h"
#include <stdio.h>
#include <string.h>

/* Global変数 */
struct CONSOLE *log;
//unsigned int *g_current_dir;
// int g_pathname_length = 0;
int console_id=0;
struct MYFILEDATA *setfdata = 0;


/* ログコンソールに文字列strを出力する */
void debug_print(char *str){
	/*	デバッグ用の出力をするときは"//"を消す
	char s[50];
	sprintf(s, "[debug] ");
	cons_putstr(log, s);
	int i;
	for(i=0; str[i]!='0' && str[i]!='\0'; i++){
		if(i == 150){
			str[i] = '0';
			break;
		}
	}

	// 150文字以上は出力しない
	if(i<150){
		cons_putstr(log, str);
	}else{
		sprintf(s, "[CAUTION:(str.length>150)]");
		cons_putstr(log, s);
		cons_putstr(log, str);
	}
	//*/
	return;
}

/**
 * manage console tasks using memory domain
 */
void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[100];
	// char s[100]; // for debug
	int path_length = 0; // for calculating cmdline
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	cons.current_dir = (struct MYDIRINFO *) ROOT_DIR_ADDR;
	cons.id = console_id;
	console_id++;
	task->cons = &cons;
	task->cmdline = cmdline;

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 未使用マーク */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 日本語フォントファイルを読み込めたか？ */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* プロンプト表示 */
	if(cons.id == 1){
		cmd_mkfs(&cons);	/* 初期コンソールに対して強制的にmkfsを使う */
	}else if(cons.id == 0){
		cmd_setlog(&cons);	/* ログ用コンソールに対してログ出力用の設定を施す */
	}
	path_length = cons_putdir(&cons);
	cons_putchar(&cons, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) { /* カーソル用タイマ */
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0); /* 次は0を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1); /* 次は1を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {	/* カーソルON */
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* カーソルOFF */
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
							cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {	/* コンソールの「×」ボタンクリック */
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
				if (i == 8 + 256) { /* バックスペース */
					if (cons.cur_x > 16 + path_length * 8) {
						/* カーソルをスペースで消してから、カーソルを1つ戻す */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) { /* if press Enter */
					/* カーソルをスペースで消してから改行する */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - (path_length) - 2] = 0;
					cons_newline(&cons);

					// *****コマンドラインのデバッグコード*****
					// sprintf(s, "original cmdline = %s[EOF]\n", cmdline);
					// cons_putstr(&cons, s);

					cons_runcmd(cmdline, &cons, fat, memtotal);	/* コマンド実行 */
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* プロンプト表示 */
					path_length = cons_putdir(&cons);
					cons_putchar(&cons, '>', 1);
				} else {
					/* 一般文字 */
					if (cons.cur_x < 240) {
						/* 一文字表示してから、カーソルを1つ進める */
						/* 多分,
						 * cons.cur_x / 8 = Ｘ文字目の文字
						 * -2 = 0文字目('>')は含めない
						 */
						cmdline[cons.cur_x / 8 - (path_length) - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			/* カーソル再表示 */
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c,
							cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

/**
 * put character in specific console.
 */
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {	/* タブ */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			//if (cons->cur_x == 8 + 240) {
			if (cons->cur_x == 8 + cons->sht->bxsize-16){
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (s[0] == 0x0a) {	/* 改行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 復帰 */
		/* とりあえずなにもしない */
	} else {	/* 普通の文字 */
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + cons->sht->bxsize-16) {
				cons_newline(cons);
			}
		}
	}
	return;
}

/**
 * make new line in specified console.
 */
void cons_newline(struct CONSOLE *cons)
{
	int x, y, xmax, ymax;
	xmax = cons->sht->bxsize - 16;
	ymax = cons->sht->bysize - 37;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	//if (cons->cur_y < 28 + 112) {
	if (cons->cur_y < 28 + ymax - 16){
		cons->cur_y += 16; /* 次の行へ */
	} else {
		/* スクロール */
		if (sheet != 0) {
			/* VRAM情報の各1行を、一つ上の場所にコピーする */
			for (y = 28; y < 28 + ymax - 16; y++) {
				for (x = 8; x < 8 + xmax; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}

			/* 最後の行を黒で塗りつぶす */
			for (y = 28 + ymax - 16; y < 28 + ymax; y++) {
				for (x = 8; x < 8 + xmax; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			/* シート内の8<x<248, 28<y<156の範囲を再描画する */
			sheet_refresh(sheet, 8, 28, 8 + xmax, 28 + ymax);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}

/**
 * consoleに文字列sを出力する
 */
void cons_putstr(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}


/**
 * consoleに文字列sを出力する
 */
void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

/**
 * consoleに文字列sを、長さlenまで出力する
 */
void cons_putstr1(struct CONSOLE *cons, char *s, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

/* 現在のディレクトリの絶対Pathを表示する.
 * @return pathnameの文字列の長さを返す(コマンドラインの長さを計算するため)
 */
int cons_putdir(struct CONSOLE *cons){
	struct MYDIRINFO *dinfo = cons->current_dir;
	char pathname[MAX_CMDLINE];
	int i;
	int pathname_length = 0;

	get_pathname(pathname, dinfo);	// パス名を探索し、pathnameに格納する
	for(i=0; pathname[i]!='\0';i++) pathname_length++;

	/* pathをコンソールに表示 (注意：このときは改行を入れない)*/
	cons_putstr(cons, pathname);
	return pathname_length;
}

/**
 * get current directory path.
 * @param pathname: set path name into pathname
 * @param dinfo: directory information
 */
void get_pathname(char *pathname, struct MYDIRINFO *dinfo){
	char s[100];
	char tempname[MAX_CMDLINE];
	char dirname[MAX_NAME_LENGTH];

	// initialize
	sprintf(pathname, "");
	while(dinfo->parent_dir != 0){
		sprintf(dirname, dinfo->name);
		sprintf(tempname, "%s/%s", dirname, pathname);
		dinfo = (struct MYDIRINFO *)dinfo->parent_dir;

		// 初期化
		sprintf(pathname, "%s", tempname);
		sprintf(dirname, "");
	}

	// pathnameに"/"(ROOT)を付け加える。
	sprintf(s, "/%s", pathname);
	sprintf(pathname, "%s", s);

	return;
}


/**
 * read command line and execute called function.
 */
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	// debug code
	//char s[30];
	//sprintf(s, "cmdline = %s[EOF]\n", cmdline);
	//cons_putstr(cons, s);

	if (strncmp(cmdline, "cat ", 4) == 0 && cons->sht != 0) {
		cmd_cat(cons, fat, cmdline);
	} else if (strncmp(cmdline, "cd ", 3) == 0){
		cmd_cd(cons, cmdline);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "edit ", 5) == 0 && cons->sht != 0){
		cmd_edit(cons, cmdline);
	} else if (strcmp(cmdline, "fddir") == 0 && cons->sht != 0) {
		cmd_fddir(cons);
	} else if (strncmp(cmdline, "fview ", 6) == 0 && cons->sht != 0){
		cmd_fview(cons, cmdline);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (strcmp(cmdline, "log") == 0 && cons->sht != 0){
		cmd_log(cons);
	} else if (strcmp(cmdline, "logcls") == 0 && cons->sht != 0){
		cmd_logcls(cons);
	} else if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "mkfs") == 0 && cons->sht != 0){
		cmd_mkfs(cons);
	}else if (strncmp(cmdline, "mkdir ", 6) == 0){
		cmd_mkdir(cons, cmdline);
	} else if (strncmp(cmdline, "mkfile ", 7) == 0){
		cmd_mkfile(cons, cmdline);
	} else if (strcmp(cmdline, "memmap") == 0 && cons->sht != 0) {
		cmd_memmap(cons, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strcmp(cmdline, "setlog") == 0 && cons->sht != 0){
		cmd_setlog(cons);
	} else if (strncmp(cmdline, "show ", 5) == 0 && cons->sht != 0){
		cmd_show(cons, cmdline);
	} else if (strcmp(cmdline, "test") == 0 && cons->sht != 0){
		cmd_test(cons);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* コマンドではなく、アプリでもなく、さらに空行でもない */
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

/**
 * command: change directory
 * cmdline = cd .. -> change parent directory
 * cmdline = cd [dir name] -> change [dir name] directory if it exists.
 */
void cmd_cd(struct CONSOLE *cons, char *cmdline){
	struct MYDIRINFO *dest_dinfo;
	char *filename = cmdline + 3;
	char *cdline = filename;

	dest_dinfo = parse_cdline(cons, cdline);

	if(dest_dinfo != 0){
		/* 構文解析に成功していたら、目的地に移動する */
		cons->current_dir = dest_dinfo;
	}
	return;
}

/**
 * cdコマンドに与えられた引数を解析する(現在開発中)
 * @param cons: console addr
 * @param cdline: characters given to cd command
 */
#define isFILENAME() (('A'<=cdline[cp] && cdline[cp]<='Z') || ('a'<=cdline[cp] && cdline[cp]<='z') || ('0'<=cdline[cp] && cdline[cp]<='9'))
#define isDOUBLEPOINT() ((cdline[cp] == '.') && (cdline[cp+1] == '.'))
int cp;

void get_dirname(char *dirname, char *cdline){
	int p;

	p=0;
	while(isFILENAME()){
		dirname[p] = cdline[cp];
		p++; cp++;
	}
	dirname[p] = '\0';
	return;
}

/* cdの引数を解析する関数*/
struct MYDIRINFO *parse_cdline(struct CONSOLE *cons, char *cdline){
	struct MYDIRINFO *dinfo;
	char s[100];
	char prev_pname[100];	//debug
	char pname[100];		//debug
	char dirname[MAX_NAME_LENGTH];
	int loop_count = 0;	//debug
	struct MYFILEINFO *finfo;

	debug_print("*****IN FUNC: parse_cdline()*****\n");

	dinfo = 0;
	cp = 0;
	sprintf(prev_pname, "N/A");

	while(cdline[cp] != '\0'){
		if(cp != 0){
			get_pathname(prev_pname, dinfo);
			loop_count++;
		}

		if(isFILENAME()){
			if(cp == 0){
				debug_print("filename has found.\n");
				/*相対パスとして処理*/
				sprintf(s, "change directory using relative path\n");
				debug_print(s);
				dinfo = cons->current_dir;
				get_pathname(prev_pname, dinfo); //debug
			}
		}else if(cdline[cp] == '/'){
			debug_print("'/' has found.\n");
			cp++;
			if(cp == 1){
				/*絶対パスとして処理*/
				sprintf(s, "change directory using absolute path\n");
				debug_print(s);
				dinfo = (struct MYDIRINFO *)ROOT_DIR_ADDR;
			}
		}else if(isDOUBLEPOINT()){
			debug_print("\"..\" has found.\n");
			cp += 2;
			if(cp == 2){
				/*相対パスとして処理*/
				sprintf(s, "change directory using relative path\n");
				debug_print(s);
				dinfo = cons->current_dir;
				get_pathname(prev_pname, dinfo);
			}

			if(dinfo->parent_dir == 0){
				cd_error(cons, "Can't move because here is ROOT directory.\n");
				debug_print("*********************************\n");
				return 0; // parse失敗
			}
			dinfo = dinfo->parent_dir;
			goto PARSE_NEXT;
		}else{
			/*エラー処理*/
			cd_error(cons, "Incorrect initial character.\n");
			debug_print("*********************************\n");
			return 0;	// parse失敗
		}

		if(isDOUBLEPOINT() || cdline[cp] == '\0'){
			// ".."または次がヌル文字なら何もしない
		}else{
			/* 指定されたディレクトリを探す */
			get_dirname(dirname, cdline);
			finfo = myfinfo_search(dirname, dinfo, MAX_FINFO_NUM);
			if(finfo == 0){
				/* 該当するディレクトリが見つからなかった */
				cd_error(cons, "Can't find this directory.\n");
				debug_print("*********************************\n");
				return 0; // parse失敗
			}else{
				/* 該当するディレクトリが見つかった */
				dinfo = finfo->child_dir;
			}
		}

		PARSE_NEXT:
		get_pathname(pname, dinfo);
		sprintf(s, "[%d]change directory: %s -> %s\n", loop_count, prev_pname, pname);
		debug_print(s);
	}

	get_pathname(prev_pname, cons->current_dir);
	get_pathname(pname, dinfo);
	sprintf(s, "[RESULT]destination: %s -> %s\n", prev_pname, pname);
	debug_print(s);

	/*ディレクトリの移動*/
	//cons->current_dir = dinfo;
	debug_print("*********************************\n");
	cons_newline(cons);
	return dinfo;
}

/* エラーの出力 */
void cd_error(struct CONSOLE *cons, char *message){
	char s[50];
	int i, j, k;

	get_pathname(s, cons->current_dir);
	for(i=0; s[i]!='\0'; i++)s[i] = ' ';	// パス名分の空白
	for(j=0; j<3; j++) s[i+j] = ' ';		// "cd "分の空白
	for(k=0; k<MAX_CMDLINE; k++){
		if(k<cp){
			s[i+j+k] = ' ';
		}else{
			s[i+j+k] = '^';
			s[i+j+k+1] = '\n';
			s[i+j+k+2] = '\0';
			break;
		}
	}
	cons_putstr(cons, s);
	cons_putstr(cons, message);
	cons_newline(cons);
	return;
}


/* コマンドライン上で簡単なファイル編集が行える関数
 * (myfopen/myfread/myfwrite/myfclose関数のテスト用) */
/* 編集モード */
#define MODE_DEF	0x00
#define MODE_CLS	0x01
#define MODE_INS	0x02
#define MODE_ADD	0x04
#define MODE_OPEN	0x08
#define MODE_ALL	0xFF
void cmd_edit(struct CONSOLE *cons, char *cmdline){
	struct MYDIRINFO *dinfo = cons->current_dir;	// open用
	struct MYFILEDATA *fdata;	// open用
	int i, p;
	char s[BODY_SIZE + BODY_SIZE_OFFSET];
	char option[15];
	char editline[50];
	int length_editline;
	unsigned int file_size;
	unsigned char mode = MODE_DEF;
	int temp_p;
	char temp_body[500];

	//sprintf(s, "cmdline = %s\n", cmdline);
	//debug_print(s);

	/* initialize */
	sprintf(option, "");
	sprintf(editline, "");

	/* command line parser */
	p = 5;
	while(cmdline[p] == ' ') p++; // 空白を読み捨てる

	if(cmdline[p] == '-'){
		/* option付きの場合、optionを取得する */
		temp_p=0;
		p++; // '-'を読み捨てる
		while(cmdline[p] != ' ' && cmdline[p] != 0){
			if(temp_p >= 10){
				/* optionの長さが大きすぎる場合, 強制終了 */
				sprintf(s, "option is too long.\n");
				cons_putstr(cons, s);
				cons_newline(cons);
				return;
			}
			option[temp_p] = cmdline[p];
			temp_p++;
			p++;
		}

		option[temp_p] = '\0';	// 終端記号の付与

		//sprintf(s, "option=%s[EOF]\n", option);
		//debug_print(s);

		/* OPEN MODEのみ最初に評価する(直後のif文で分岐路が大幅に減るから) */
		if(strcmp(option, "open") == 0){
			/* 指定されたファイルを開く */
			debug_print("EDIT:open mode\n");
			mode = MODE_OPEN;
		}

		if(setfdata == 0 && mode != MODE_OPEN){
			/* まだ編集用のファイルが無い場合、何もせずに終了 */
			sprintf(s, "can't edit: There is no file being opened.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}

		if(strcmp(option, "cls") == 0){
			/* ファイルをクリアした後、editlineを入力する */
			debug_print("EDIT:clear mode\n");
			mode = MODE_CLS;
			myfwrite(setfdata, "");	// 引数がない場合もあるので、ここでもクリアする
		}else if(strcmp(option, "ins") == 0){
			/* ファイルの重複部分のみ上書きする */
			debug_print("EDIT:insert mode\n");
			mode = MODE_INS;
		}else if(strcmp(option, "add") == 0){
			/* ファイルの末尾にeditlineを追加する */
			debug_print("EDIT:add mode\n");
			mode = MODE_ADD;
		}else if(strcmp(option, "show") == 0){
			debug_print("EDIT:show mode\n");
			myfread(temp_body, setfdata);
			sprintf(s, "setfdata->head.stat=0x%02x\n", setfdata->head.stat);
			debug_print(s);
			sprintf(s, "%s[EOF]\n", temp_body);
			cons_putstr(cons, s);
			sprintf(s, "size: %d[byte]\n", get_size_myfdata(setfdata));
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}else if(strcmp(option, "close") == 0){
			debug_print("EDIT:close mode\n");
			myfclose(setfdata);
			setfdata = 0;
			sprintf(s, "opened file was closed.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}else if(strcmp(option, "save") == 0){
			if(myfsave(setfdata) == -1){
				sprintf(s, "Can't save because of error in myfinfo_search() in myfsave()\n");
				cons_putstr(cons, s);
				cons_newline(cons);
			}else{
				sprintf(s, "finished saving opened file.\n");
				cons_putstr(cons, s);
				cons_newline(cons);
				return;
			}
		}else if(strcmp(option, "open") == 0){
			// 何もせずに終了
		}else{
			/* 例外処理(強制終了) */
			sprintf(s, "invalid option.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}
		if(cmdline[p] == ' ') p++; //空白ならポインタを次に進める

	}else if(setfdata == 0 && mode != MODE_OPEN){
		/* optionがない場合にも例外処理を適用する */
		/* まだ編集用のファイルが無い場合、何もせずに終了 */
		sprintf(s, "Can't edit: There is no file being opened.\n");
		cons_putstr(cons, s);
		cons_newline(cons);
		return;
	}

	/* edit lineの取得 */
	temp_p = 0;
	while(cmdline[p] != 0 && cmdline[p] != '\0'){
		if(temp_p >=48){
			/* 編集文字列が長い場合(現状ではcmdlineがたかだか50しかない) */
			sprintf(s, "edit line is too long.");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}
		editline[temp_p] = cmdline[p];
		temp_p++;
		p++;
	}
	length_editline = temp_p;
	editline[temp_p] = '\0';	// 終端記号の付与
	/**** end of parser ****/

	/* 取得したoptionの応じて、editlineの編集モードを変える */
	char temp[1024];	//編集用の文字列上限は1024文字(要検討)
	if(mode == MODE_DEF || mode == MODE_ADD){
		/* defaultはaddモードと同じ */
		myfread(s, setfdata);
		strcat(s, editline);
		debug_print("char *s out of myfwrite() is shown below.\n");
		debug_print(s);
		debug_print("\n");
		myfwrite(setfdata, s);

	}else if(mode == MODE_CLS){
		/* CLEAR SCREEN MODE
		 * バッファの内容を消去し、editlineの内容を書き込む。*/
		myfread(temp, setfdata);
		sprintf(temp, "");
		strcat(temp, editline);
		myfwrite(setfdata, temp);	// クリア&上書き

	}else if(mode == MODE_INS){
		int nullFlag = 0;
		/* myfwriteで任意の一文字を置き換える関数を用意する必要がある(！！！要検討！！！) */
		myfread(temp, setfdata);
		for(i=0; i<length_editline; i++){
			if(editline[i] != ' '){
				/* 編集文字列に何らかの文字が入力されている場合 */
				temp[i] = editline[i];
			}else if(temp[i] == '\0'){
				/* ファイルデータのEOFが来たら空白に置き換える */
				temp[i] = ' ';
				nullFlag = 1;
			}
		}
		if(nullFlag == 1)temp[i] = '\0';	// 文字列の末尾にヌル文字を追加
		sprintf(s, "INS MODE RESULT: %s\n", temp);
		debug_print(s);
		myfwrite(setfdata, temp);

	}else if(mode == MODE_OPEN){
		/* ファイルを開く */
		if(setfdata != 0){
			/* 既に編集中のファイルがある場合、何もせずに終了 */
			sprintf(s, "can't open: There is a file being opened.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}
		fdata = myfopen(editline, dinfo);
		if(fdata == 0){
			/* ファイルが開けなかった場合 */
			sprintf(s, "can't open \"%s\".\n", editline);
			cons_putstr(cons, s);
			cons_newline(cons);

		}else{
			/* ファイルが正常に開けた場合 */
			sprintf(s, "opened \"%s\" file.\n", editline);
			cons_putstr(cons, s);
			cons_newline(cons);
			setfdata = fdata;
		}
		return;
	}else{
		/* 例外処理 */
		debug_print("unexpected error at edit mode.\n");
	}

	myfread(temp_body, setfdata);
	sprintf(s, "edit result:\n%s[EOF]\n", temp_body);
	cons_putstr(cons, s);

	/* ファイルサイズの取得 */
	file_size = get_size_myfdata(setfdata);
	sprintf(s, "size: %d[byte]\n", file_size);
	cons_putstr(cons, s);
	cons_newline(cons);

	return;
}

/**
 * make directory in my filesystem
 */
void cmd_mkdir(struct CONSOLE *cons, char *cmdline){
	int i, j;
	char s[50];
	char *dir_name;
	struct MYDIRINFO *dinfo = cons->current_dir;
	struct MYFILEINFO *finfo; // for debug

	dir_name = cmdline + 6;

	/* ディレクトリ名の文字列上限を超えた/既に同じディレクトリ名が存在している場合は何もしない */
	for(i=0; dir_name[i] != 0; i++);
	if(i > 8){
		sprintf(s, "directory name shoule be within 8 letters.\n");
		cons_putstr(cons, s);
		cons_newline(cons);
		return;
	}else if((finfo = myfinfo_search(dir_name, dinfo, MAX_FINFO_NUM)) != 0){
		sprintf(s, "this directory name is already used, please use other name.\n");
		cons_putstr(cons, s);

		sprintf(s, "\tname = %s\n\text = %s\n\ttype = 0x%02x\n", finfo->name, finfo->ext, finfo->type);
		debug_print(s);
		cons_newline(cons);
		return;
	}

	/* ファイルの最後尾までiを進める */
	for (i = 0; i < MAX_FINFO_NUM; i++) {
		/* もうFileがこれ以上存在しない場合, breakする */
		if (dinfo->finfo[i].name[0] == 0x00) {
			break;
		}
	}

	/* 小文字は大文字に直す */
	for(j=0; dir_name[j] != 0; j++) {
		if ('a' <= dir_name[j] && dir_name[j] <= 'z') {
			dir_name[j] -= 'a'-'A';
		}
		// dir_nameの文字列分を一文字ずつ格納する
		dinfo->finfo[i].name[j] = dir_name[j];
	}

	// 残りの文字列を空白で埋める
	for(; j<8 ;j++) dinfo->finfo[i].name[j] = ' ';

	// dir_nameの文字列分を一文字ずつ格納する
	//for(j=0; dir_name[j] != 0; j++) dinfo->finfo[i].name[j] = dir_name[j];

	/* ファイル情報の新規作成 */
	dinfo->finfo[i].child_dir = get_newdinfo();	// 次のmydirの場所を格納
	dinfo->finfo[i].clustno = 0;
	dinfo->finfo[i].date = 0;
	dinfo->finfo[i].type = 0x10;
	dinfo->finfo[i].size = sizeof(dinfo->finfo[i]);
	dinfo->finfo[i].fdata = 0;	// ０：ファイルデータが存在しない

	/* 作成したファイル情報の出力 */
	sprintf(s, "created directory: name = %s\n", dinfo->finfo[i].name);
	cons_putstr(cons, s);
	sprintf(s, "\tchild dir address = 0x%08x\n", dinfo->finfo[i].child_dir);
	debug_print(s);
	sprintf(s, "\ttype=0x%02x\n", dinfo->finfo[i].type);
	debug_print(s);

	/* 子ディレクトリをMYDIRINFOでマウントする。(dinfoがchild_dinfoの親ディレクトリ) */
	struct MYDIRINFO *child_dinfo = (struct MYDIRINFO *)dinfo->finfo[i].child_dir;
	child_dinfo->this_dir = dinfo->finfo[i].child_dir; // 子のaddr ← 親のdir addr
	sprintf(child_dinfo->name, dir_name); // ディレクトリ名を格納
	child_dinfo->parent_dir = dinfo->this_dir; // 子の親addr ← 親addr

	/* 子ディレクトリ情報の出力(デバッグ用) */
	sprintf(s, "\tchild dinfo addr = 0x%08x\n", child_dinfo->this_dir);
	debug_print(s);
	sprintf(s, "\tchild dinfo name = %s\n", child_dinfo->name);
	debug_print(s);
	sprintf(s, "\tchild dinfo parent addr = 0x%08x\n", child_dinfo->parent_dir);
	debug_print(s);

	cons_newline(cons);	// 改行
	return;
}

/**
 * make file in my filesystem
 */
void cmd_mkfile(struct CONSOLE *cons, char *cmdline){
	/* (0x0010 + 0x0026)の場所にある情報を, FILEINFOのデータ構造として, メモリ上にコピーする. */
	struct MYDIRINFO *dinfo = cons->current_dir;
	int i, j;
	char s[50];
	char *name = cmdline + 7;
	char filename[12];
	struct MYFILEINFO *finfo;

	/* filenameの整形 */
	for (j = 0; j < 11; j++) {
		filename[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		/* ファイル名の文字列上限を超えた場合は何もしない */
		if (j >= 11) {
			sprintf(s, "file name should be within 8 letters,\n");
			cons_putstr(cons, s);
			sprintf(s, "and extension should be within 3 letters.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
			return;
		}
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			filename[j] = name[i];
			if ('a' <= filename[j] && filename[j] <= 'z') {
				/* 小文字は大文字に直す */
				filename[j] -= 'a'-'A';
			}
			j++;
		}
	}

	/* 既に同じディレクトリ名が存在している場合, 何もしない*/
	if((finfo = myfinfo_search(filename, dinfo, MAX_FINFO_NUM)) != 0){
		sprintf(s, "this file name is already used, please use other name.\n");
		cons_putstr(cons, s);
		///* debug code: Viewing already used file name
		sprintf(s, "\tname = %s\n\text = %s\n\ttype = 0x%02x\n", finfo->name, finfo->ext, finfo->type);
		cons_putstr(cons, s);
		cons_newline(cons);
		//*/
		cons_newline(cons);
		return;
	}

	/***** 新しいファイルを作成する(今はtextファイルしか作れない) *****/
	for (i = 0; i < MAX_FINFO_NUM; i++) {
		/* もうFileがこれ以上存在しない場合, breakする */
		if (dinfo->finfo[i].name[0] == 0x00) {
			break;
		}
	}

	/* ファイル情報を書き込む */
	for(j=0; j<8; j++) dinfo->finfo[i].name[j] = filename[j];
	for(j=0; j<3; j++) dinfo->finfo[i].ext[j] = filename[8+j];
	dinfo->finfo[i].clustno = 0;
	dinfo->finfo[i].date = 0;
	dinfo->finfo[i].type = 0x20;	/* ファイル属性は適当に0x20とした(他のもなぜか0x20だから)*/
	dinfo->finfo[i].size = 0;

	/* ファイルデータの生成 */
	struct MYFILEDATA test;	//要検討
	test.head.stat = STAT_ALL;	// 新規ファイル作成時はステータスビットを全て立てる(要検討)
	test.head.this_dir = dinfo->this_dir;//要検討
	struct MYFILEDATA *fdata = get_newfdata(&test); // 新規データを取得　//要検討
	//strcpy(fdata->head.name, name);	// ファイルデータにも名前をコピー("name"であることに注意！)

	/***** debug *****/
	//sprintf(s, "fdata->head.name =\t\t%s[EOF]\n", fdata->head.name);
	//debug_print(s);
	//sprintf(s, "dinfo->finfo[i].name =\t%s[EOF]\n", dinfo->finfo[i].name);
	//debug_print(s);
	//struct MYFILEINFO *debug = myfinfo_search(fdata->head.name, dinfo, MAX_FINFO_NUM);
	//sprintf(s, "test(should not be 0) = %d\n", debug);
	//debug_print(s);
	/*****************/
	fdata->head.stat = 0x01;	// valid bitを立てる
	fdata->head.this_fdata = fdata; // データ領域の一番目に確保する
	fdata->head.this_dir = dinfo->this_dir;
	dinfo->finfo[i].fdata = fdata;	// デフォルトは０(要検討！)

	// debug code: Viewing status of created file.
	sprintf(s, "created file name = %s[EOF]\n", filename);
	cons_putstr(cons, s);

	sprintf(s, "file name = %s\n", dinfo->finfo[i].name);
	cons_putstr(cons, s);
	sprintf(s, "file type=0x%02x\n", dinfo->finfo[i].type);
	cons_putstr(cons, s);
	cons_newline(cons);
	return;
}

/**
 * given console becomes console for log
 */
void cmd_setlog(struct CONSOLE *cons){
	char s[100];
	log = cons;
	sprintf(s, "    log cons\nmem:%d %d\nsht:%d %d\n", &log, &cons, log->sht, cons->sht);
	cons_putstr0(log, s);
	cons_newline(log);
	return;
}

/**
 * show total memory
 */
void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int i;
	char s[60];
	sprintf(s, "frees  %d\n", memman->frees);
	cons_putstr0(cons, s);
	for(i=0; i< memman->frees; i++){
		sprintf(s, "(%d) size=%dKB addr=0x%x\n", i, memman->free[i].size/1024, memman->free[i].addr);
		cons_putstr(cons, s);
	}
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	return;
}

void cmd_memmap(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	return;
}

/**
 * show something in log console
 */
void cmd_log(struct CONSOLE *cons){
	char s[100];
	sprintf(s, "log test");
	cons_putstr0(log, s);
	cons_newline(log);
	return;
}

/**
 * ???
 */
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct MYFILEINFO *finfo = myfinfo_search(cmdline + 4, cons->current_dir, MAX_FINFO_NUM);
	char *p;
	char s[30];
	sprintf(s, "filename=%s\n", finfo->name);
	cons_putstr(cons, s);

	if (finfo != 0) {
		/* ファイルが見つかった場合 */
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		cons_putstr1(cons, p, finfo->size);
		memman_free_4k(memman, (int) p, finfo->size);
	} else {
		/* ファイルが見つからなかった場合 */
		cons_putstr0(cons, "File not found.\n");
	}
	cons_newline(cons);
	return;
}

/**
 * clear screen in console
 */
void cmd_cls(struct CONSOLE *cons)
{
	int x, y, xmax, ymax;
	struct SHEET *sheet = cons->sht;
	xmax = sheet->bxsize-16;
	ymax = sheet->bysize-37;
	for (y = 28; y < 28 + ymax; y++) {
		for (x = 8; x < 8 + xmax; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + xmax, 28 + ymax);
	cons->cur_y = 28;
	return;
}

/**
 * making large console for log
 */
void cmd_logcls(struct CONSOLE *cons){
	int x, y, xmax, ymax;
	struct SHEET *sheet = log->sht;
	xmax = sheet->bxsize-16;
	ymax = sheet->bysize-37;
	for (y = 28; y < 28 + ymax; y++) {
		for (x = 8; x < 8 + xmax; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + xmax, 28 + ymax);
	log->cur_y = 28;
	return;
}

/**
 * test function for command.
 */
void cmd_test(struct CONSOLE *cons){
	char s[100];

	sprintf(s, "BLOCK_SIZE = %d\n", BLOCK_SIZE);
	debug_print(s);
	sprintf(s, "sizeof(struct HEAD) = %d\n", sizeof(struct HEAD));
	debug_print(s);
	sprintf(s, "BODY_SIZE = BLOCK_SIZE - sizeof(struct HEAD) = %d\n", BODY_SIZE);
	debug_print(s);

	/*
	char alpha[2];
	alpha[0] = 'A';
	alpha[1] = '\0';
	while('A' <= alpha[0] && alpha[0] <= 'z'){
		sprintf(s, "alpha = %s(%d)\n", alpha, alpha[0]);
		debug_print(s);
		alpha[0]++;
	}
	 */

	cons_newline(cons);
	return;
}

/**
 * show  status about a specific file.
 * @param cmdline is "show [file name]"
 */
void cmd_show(struct CONSOLE *cons, char *cmdline){
	char s[150];
	struct MYDIRINFO *dinfo = cons->current_dir;
	char *filename = cmdline + 5;
	struct MYFILEINFO *finfo = myfinfo_search(filename, dinfo, MAX_FINFO_NUM);

	///* for debug
	sprintf(s, "\tfinfo addr = 0x%08x\n", finfo);
	cons_putstr(cons, s);
	//*/

	if(finfo == 0){
		sprintf(s, "\t%s was not found.\n", filename);
		cons_putstr(cons, s);
		sprintf(s, "\tfinfo number was %d.\n", finfo);
		cons_putstr(cons, s);
		cons_newline(cons);
		return;

		/* if finfo was found */
	}else{
		sprintf(s, "\tname=%s[EOF]\n\text=%s[EOF]\n\tclustno=%d\n\tdate=%d\n\tsize=%d[byte]\n\ttime=%d\n\ttype=0x%02x\n",
				finfo->name,
				finfo->ext,
				finfo->clustno,
				finfo->date,
				finfo->size,
				finfo->time,
				finfo->type);
		cons_putstr(cons, s);
	}
	/* show the strings in char s[100] */
	// show detail file type info(文字列sの出力方式はあっている（確認済み）)
	unsigned char filetype = finfo->type;
	sprintf(s, "\tdetail file type: ");
	if((filetype & FTYPE_DIR) != 0x00)	sprintf(s, "%s directory/readonly ", s);
	if((filetype & FTYPE_FILE) != 0x00)	sprintf(s, "%s file ", s);
	if((filetype & FTYPE_SYS) != 0x00) sprintf(s, "%s system ", s);
	if((filetype & FTYPE_OTHER) != 0x00)	sprintf(s, "%s other ", s);
	sprintf(s, "%s\n", s);
	cons_putstr0(cons, s);
	cons_newline(cons);
	return;
}

/**
 * view file data including header.
 * @param cmdline is "fview [file name]"
 */
void cmd_fview(struct CONSOLE *cons, char *cmdline){
	char s[130];
	struct MYDIRINFO *dinfo = cons->current_dir;
	char *filename = cmdline + 6;
	struct MYFILEINFO *finfo = myfinfo_search(filename, dinfo, MAX_FINFO_NUM);

	///* for debug */
	sprintf(s, "finfo addr = 0x%08x\n", finfo);
	debug_print(s);
	//*/

	if(finfo == 0){
		sprintf(s, "\t%s was not found.\n", filename);
		cons_putstr(cons, s);
		sprintf(s, "\tfinfo number was %d.\n", finfo);
		cons_putstr(cons, s);
		cons_newline(cons);
		return;

		/* if finfo was found */
	}else{
		if(finfo->fdata != 0){
			/* ファイルデータが存在した場合 */
			sprintf(s, "fdata addr = 0x%08x\n", finfo->fdata);
			debug_print(s);
			sprintf(s, "head.data_addr=0x%08x\n", finfo->fdata->head.this_fdata);
			debug_print(s);
			sprintf(s, "head.dir_addr=0x%08x\n",	finfo->fdata->head.this_dir);
			debug_print(s);
			sprintf(s, "head.stat=0x%02x\n", finfo->fdata->head.stat);
			debug_print(s);

			myfread(s, finfo->fdata);
			cons_putstr(cons, s);
			cons_putstr(cons, "[EOF]\n");	// body部の最後には改行がないので挿入
			sprintf(s, "size: %d\n", get_size_myfdata(finfo->fdata));
			cons_putstr(cons, s);
			cons_newline(cons);
		}else{
			/* ファイルデータがない場合(Ex. ディレクトリ) */
			sprintf(s, "\tthis is not file.\n");
			cons_putstr(cons, s);
			cons_newline(cons);
		}
	}

	/* show the strings in char s[100] */
	return;
}


/* FDに格納されている全てのファイルを調べ, 表示させる関数 */
void cmd_mkfs(struct CONSOLE * cons){
	struct MYDIRINFO dinfo;
	/* (0x0010 + 0x0026)の場所にある情報を, FILEINFOのデータ構造として, メモリ上にコピーする. */
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];

	sprintf(dinfo.name, "ROOT    ");
	dinfo.parent_dir = 0x00; // 0x00: root directory
	dinfo.this_dir = (struct MYDIRINFO *)ROOT_DIR_ADDR;
	cons->current_dir = (struct MYDIRINFO *)dinfo.this_dir;

	/* とりあえず10ファイルだけコピーしてみる */
	for (i = 0; i < 10; i++) {
		/* もうFileがこれ以上存在しない場合, breakする */
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		/* 0xe5:既に消去されたファイル */
		if (finfo[i].name[0] != 0xe5) {
			/* 0x18 = 0x10 + 0x18
			 * 0x10:ディレクトリ
			 * 0x08:ファイルではない情報(ディスクの名前とか)
			 * よって、File Typeが"ファイル"であった場合 */
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j]; /* "filename"をファイルネームで上書き */
				}
				s[ 9] = finfo[i].ext[0];	/* "e"を上書き */
				s[10] = finfo[i].ext[1];	/* "x"を上書き */
				s[11] = finfo[i].ext[2];	/* "t"を上書き */

				dinfo.finfo[i].clustno = finfo[i].clustno;
				dinfo.finfo[i].date = finfo[i].date;
				for(j=0;j<3 ;j++) dinfo.finfo[i].ext[j] = finfo[i].ext[j];
				for(j=0;j<8 ;j++) dinfo.finfo[i].name[j] = finfo[i].name[j];
				for(j=0;j<10;j++) dinfo.finfo[i].reserve[j] = finfo[i].reserve[j];
				dinfo.finfo[i].size = finfo[i].size;
				dinfo.finfo[i].time = finfo[i].time;
				dinfo.finfo[i].type = finfo[i].type;

				cons_putstr0(cons, s);
			}
		}
	}
	// memcpy(dinfo.finfo, finfo, sizeof(finfo));
	memcpy((unsigned int *)ROOT_DIR_ADDR, &dinfo, sizeof(dinfo));
	sprintf(s, "make filesystem!\n");
	cons_putstr0(cons, s);
	cons_newline(cons);
	return;
}

/**
 * show files information in new filesystem
 */
void cmd_dir(struct CONSOLE *cons){
	/* (0x0010 + 0x0026)の場所にある情報を, FILEINFOのデータ構造として, メモリ上にコピーする. */
	struct MYDIRINFO *dinfo = cons->current_dir;
	int i, j;
	char s[50];

	/* show related directory */
	sprintf(s, "directory name\t%s\n", dinfo->name);
	cons_putstr(cons, s);
	sprintf(s, "current dir addr\t0x%08x\n", dinfo->this_dir);
	debug_print(s);
	sprintf(s, "parent dir addr\t0x%08x\n", dinfo->parent_dir);
	debug_print(s);

	// search present my directory
	// int dir_num;
	// dir_num = get_newdinfo(cons);	// デバッグのために&consをしている。

	// debug code: 次にマウントするMYDIRINFOの番号を表示
	//sprintf(s, "dinfo number = %d\n", dir_num);
	//cons_putstr(cons, s);

	//display files in current dir
	for (i = 0; i < MAX_FINFO_NUM; i++) {
		/* もうFileがこれ以上存在しない場合, breakする */
		if (dinfo->finfo[i].name[0] == 0x00) {
			break;
		}

		/* 0xe5:既に消去されたファイル */
		if (dinfo->finfo[i].name[0] != 0xe5) {
			/* 0x18 = 0x10 + 0x18
			 * 0x10:ディレクトリ
			 * 0x08:ファイルではない情報(ディスクの名前とか)
			 */
			/* File Typeが"ファイル"の場合 */
			if ((dinfo->finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext\t%7d [FILE]\n", dinfo->finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = dinfo->finfo[i].name[j]; /* "filename"をファイルネームで上書き */
				}
				s[ 9] = dinfo->finfo[i].ext[0];	/* "e"を上書き */
				s[10] = dinfo->finfo[i].ext[1];	/* "x"を上書き */
				s[11] = dinfo->finfo[i].ext[2];	/* "t"を上書き */
				cons_putstr(cons, s);

				/* File Typeが"ディレクトリ"の場合 */
			}else if((dinfo->finfo[i].type & 0x10) == 0x10){
				sprintf(s, "filename    \t%7d [DIR]\n", dinfo->finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = dinfo->finfo[i].name[j]; /* "filename"をファイルネームで上書き */
				}
				cons_putstr(cons, s);
				//sprintf(s, "test %s\t%d\t[DIR]",dinfo->finfo[i].name, dinfo->finfo[i].size);
				//cons_putstr(cons, s);
			}
		}
	}

	/* ひとつもファイルが見つからなかった時の処理 */
	if(i == 0){
		sprintf(s, "this directory has no file...\n");
		cons_putstr(cons, s);
	}
	cons_newline(cons);
	return;
}

/**
 * show file information in old filesystem
 */
void cmd_fddir(struct CONSOLE *cons)
{
	/* (0x0010 + 0x0026)の場所にある情報を, FILEINFOのデータ構造として, メモリ上にコピーする. */
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < MAX_FINFO_NUM; i++) {
		/* もうFileがこれ以上存在しない場合, breakする */
		if (finfo[i].name[0] == 0x00) {
			break;
		}

		/* 0xe5:既に消去されたファイル */
		if (finfo[i].name[0] != 0xe5) {
			/* 0x18 = 0x10 + 0x18
			 * 0x10:ディレクトリ
			 * 0x08:ファイルではない情報(ディスクの名前とか)
			 * よって、File Typeが"ファイル"であった場合 */
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j]; /* "filename"をファイルネームで上書き */
				}
				s[ 9] = finfo[i].ext[0];	/* "e"を上書き */
				s[10] = finfo[i].ext[1];	/* "x"を上書き */
				s[11] = finfo[i].ext[2];	/* "t"を上書き */
				cons_putstr0(cons, s);
				sprintf(s, "filetype=0x%02x\n", finfo[i].type);
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

/**
 * ??? delete console
 */
void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();
	if (cons->sht != 0) {
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);	/* 768〜1023 */
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024〜2023 */
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

/**
 * make new thread to start new application
 */
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

/**
 * ???
 */
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	/* コマンドラインに入力された文字列を、一文字ずつ新しいコンソールに入力 */
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

/**
 * change language mode
 */
void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}

/**
 * コマンドラインの入力文字列が
 * APIによるコールであるかどうかを調べる関数
 */
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET *sht;

	/* コマンドラインからファイル名を生成 */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; /* とりあえずファイル名の後ろを0にする */

	/* ファイルを探す */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/* 見つからなかったので後ろに".HRB"をつけてもう一度探してみる */
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		/* ファイルが見つかった場合 */
		appsiz = finfo->size;	// アプリケーションのサイズを取得
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			/* サイズが36byte以上 && Header == "Hari" && ファイルのロードが成功した場合 */
			segsiz = *((int *) (p + 0x0000));	// セグメントのサイズを取得
			esp    = *((int *) (p + 0x000c));	// レジスタESPを取得
			datsiz = *((int *) (p + 0x0010));	// データサイズを取得
			dathrb = *((int *) (p + 0x0014));	// データの内容を取得
			q = (char *) memman_alloc_4k(memman, segsiz);	// メモリ領域を確保
			task->ds_base = (int) q;	// task
			set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					/* アプリが開きっぱなしにした下じきを発見 */
					sheet_free(sht);	/* 閉じる */
				}
			}
			for (i = 0; i < 8; i++) {	/* クローズしてないファイルをクローズ */
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/* ファイルが見つからなかった場合 */
	return 0;
}

/**
 * use haribote api
 * @param edx defines type of command
 */
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1;	/* eaxの次の番地 */
	/* 保存のためのPUSHADを強引に書き換える */
	/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
	/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	switch(edx){
	case 1:
		cons_putchar(cons, eax & 0xff, 1);
		break;
	case 2:
		cons_putstr0(cons, (char *) ebx + ds_base);
		break;
	case 3:
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
		break;
	case 4:
		return &(task->tss.esp0);
		break;
	case 5:
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		sheet_updown(sht, shtctl->top); /* 今のマウスと同じ高さになるように指定： マウスはこの上になる */
		reg[7] = (int) sht;
		break;
	case 6:
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
		break;
	case 7:
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
		break;
	case 8:
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;	/* 16バイト単位に */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
		break;
	case 9:
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
		break;
	case 10:
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16バイト単位に切り上げ */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
		break;
	case 11:
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
		break;
	case 12:
		sht = (struct SHEET *) ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
		break;
	case 13:
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			if (eax > esi) {
				i = eax;
				eax = esi;
				esi = i;
			}
			if (ecx > edi) {
				i = ecx;
				ecx = edi;
				edi = i;
			}
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
		break;
	case 14:
		sheet_free((struct SHEET *) ebx);
		break;
	case 15:
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	/* FIFOが空なので寝て待つ */
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* カーソル用タイマ */
				/* アプリ実行中はカーソルが出ないので、いつも次は表示用の1を注文しておく */
				timer_init(cons->timer, &task->fifo, 1); /* 次は1を */
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* カーソルON */
				cons->cur_c = COL8_FFFFFF;
			}
			if (i == 3) {	/* カーソルOFF */
				cons->cur_c = -1;
			}
			if (i == 4) {	/* コンソールだけを閉じる */
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);	/* 2024〜2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256) { /* キーボードデータ（タスクA経由）など */
				reg[7] = i - 256;
				return 0;
			}
		}
		break;
	case 16:
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	/* 自動キャンセル有効 */
		break;
	case 17:
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
		break;
	case 18:
		timer_settime((struct TIMER *) ebx, eax);
		break;
	case 19:
		timer_free((struct TIMER *) ebx);
		break;
	case 20:
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}

		break;

	case 21:
		/* int api_fopen(char *fname):ファイルのオープン
		 * EDX = 21
		 * EBX = file name
		 * EAX = file handle (return 0 if file open failed.)
		 */
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char *) ebx + ds_base,
					(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int) fh;
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
		}
		break;
	case 22:
		/* void api_fclose(int fhandle): ファイルのクローズ
		 * EDX = 22
		 * EAX = file handle
		 */
		fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
		break;
	case 23:
		/* api_fseek(int fhandle, int offset, int mode):ファイルのシーク
		 * EDX = 23
		 * EAX = file handle
		 * ECX = seek mode
		 * 		0:シークの原点はファイルの先頭
		 * 		1:シークの原点は現在のアクセス位置
		 * 		2:シークの原点はファイルのs終端
		 * EBX = シーク量
		 */
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
		break;
	case 24:
		/* int api_fsize(int fhandle, int mode): ファイルサイズの取得
		 * EDX = 24
		 * EAX = filehandle
		 * ECX = ファイルサイズ取得モード
		 * 		0:普通のファイルサイズ
		 * 		1:現在の読み込み位置はファイル戦闘から何バイト目か
		 * 		2:ファイル終端からみた現在位置までのバイト数
		 * EAX = return file size
		 */
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
		break;
	case 25:
		/* int api_fread():
		 * EDX = 25
		 * EAX = file handle
		 * EBX = バッファの番地
		 * ECX = 最大読み込みバイト数
		 * EAX = return 今回読みためたバイト数 */
		fh = (struct FILEHANDLE *) eax;
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
		break;
	case 26:
		/* int api_cmdline(char *buf, int maxsize):コマンドラインの取得
		 * EDX = 26
		 * EBX = コマンドラインを格納する番地
		 * ECX = 何バイトまで格納できるか
		 * EAX = return 何バイト格納したか */
		i = 0;
		for (;;) {
			*((char *) ebx + ds_base + i) =  task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
		break;
	case 27:
		/* inr api_getlang(void): langmodeの取得
		 * EDX = 27
		 * EAX = return langmode;
		 */
		reg[7] = task->langmode;
		break;
	case 28:
		break;
	case 29:
		break;
	case 30:
		break;
	default :
		break;
	}

	return 0;
}

/**
 * ??? interface handler
 */
int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 異常終了させる */
}

/**
 * ??? interface handler
 */
int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* 異常終了させる */
}

/**
 * ??? interface handler
 */
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}
