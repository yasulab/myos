#include "apilib.h"

void HariMain(void)
{
	int fh;
	char c, cmdline[30], *p;

	api_cmdline(cmdline, 30);
	for (p = cmdline; *p > ' '; p++) { }	/* スペースが来るまで読み飛ばす */
	for (; *p == ' '; p++) { }	/* スペースを読み飛ばす */
	fh = api_fopen(p);
	if (fh != 0) {
		for (;;) {
			/* ファイルを読んでいき, 終了記号(0)が出現したら終了 */
			if (api_fread(&c, 1, fh) == 0) {
				break;
			}

			api_putchar(c);
		}
	} else {
		api_putstr0("File not found.\n");
	}
	api_end();
}
