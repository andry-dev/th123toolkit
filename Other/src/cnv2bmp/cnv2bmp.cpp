#include <windows.h>
#include <stdio.h>


char get_bits (char *name);

void get_palette (RGBQUAD palette[256], bool pal2p);

bool convert8 (char *name, RGBQUAD palette[256]);
bool convert32 (char *name);


int main (int argc, char *argv[])
{
	RGBQUAD palette[256];

	// カラーパレット取得
	if ((argc > 1) && (strcmp (argv[1], "-2p") == 0))
		get_palette (palette, true);
	else
		get_palette (palette, false);

	HANDLE hFind = NULL;
	WIN32_FIND_DATA wfd;

	if ((hFind = FindFirstFile ("*.cnv", &wfd)) != INVALID_HANDLE_VALUE) {
		do {
			printf ("%s -> ", wfd.cFileName);

			// 画像情報チェック（色数、サイズ）
			char bits;
			if ((bits = get_bits (wfd.cFileName)) < 0) {
				printf ("cnv convert failed.\n");
				continue;
			}

			if (bits == 8) {
				if (!convert8 (wfd.cFileName, palette)) {
					printf ("cnv convert failed.\n");
					continue;
				}
			} else {
				if (!convert32 (wfd.cFileName)) {
					printf ("cnv convert failed.\n");
					continue;
				}
			}

			printf ("OK\n");
		} while (FindNextFile (hFind, &wfd));
	} else {
		printf ("cnv file search failed.\n");
		return 0;
	}

	FindClose (hFind);

	return 0;
}


char get_bits (char *name)
{
	FILE *cnv = fopen (name, "rb");
	if (cnv == NULL) return -1;

	char bits;
	int width, height, pitch;
	fread (&bits, 1, 1, cnv);
	fread (&width, 4, 1, cnv);
	fread (&height, 4, 1, cnv);
	fread (&pitch, 4, 1, cnv);

	fclose (cnv);

	// 未対応の色数
	if ((bits != 8) && (bits != 24) && (bits != 32))
		return -1;

	// 未対応のサイズ（背景画像でも1400*1200前後）
	if ((width > 2000) || (height > 2000) || (width < 0) || (height < 0))
		return -1;

	return bits;
}


void get_palette (RGBQUAD palette[256], bool pal2p)
{
	FILE *pal = NULL;
	WORD buf[256];
	memset (palette, 0, 1024);

	// 1Pパレットデータ取得
	pal = fopen ((pal2p) ? "palette001.pal" : "palette000.pal", "rb");
	if (pal == NULL) {
		palette[0].rgbReserved = 0xFF;
		return;
	}
	fseek (pal, 1, SEEK_SET);
	fread (buf, 2, 256, pal);
	fclose (pal);

	// パレットデータを16bitsから24bitsに変換
	for (int i = 0; i < 256; i++) {
		float r, g, b;
		b = (float)(buf[i] & 0x1F);
		g = (float)((buf[i] & 0x3E0) >> 5);
		r = (float)((buf[i] & 0x7C00) >> 10);

		palette[i].rgbBlue = (byte)(b *8.25);
		palette[i].rgbGreen = (byte)(g *8.25);
		palette[i].rgbRed = (byte)(r *8.25);
	}
}


bool convert8 (char *name, RGBQUAD palette[256])
{
	if (palette[0].rgbReserved == 0xFF)
		return false;

	FILE *cnv = fopen (name, "rb");

	int width, height, pitch;
	fseek (cnv, 1, SEEK_SET);
	fread (&width, 4, 1, cnv);
	fread (&height, 4, 1, cnv);
	fread (&pitch, 4, 1, cnv);
	fseek (cnv, 0x11, SEEK_SET);

	char *buf = new char[pitch*height];
	fread (buf, 1, pitch*height, cnv);

	fclose (cnv);

	BITMAPFILEHEADER bf = {0x4D42, (pitch*height)+0x436, 0, 0, 0x436};
	BITMAPINFOHEADER bi = {0x28, pitch, height, 1, 8, BI_RGB, pitch*height, 0, 0, 0, 0};

	// 出力ファイル名作成
	char sbmp[128];
	strcpy (sbmp, name);
	strcpy (strrchr (sbmp, '.'), ".bmp");

	FILE *bmp = fopen (sbmp, "wb");
	if (bmp == NULL) {
		delete [] buf;
		return false;
	}

	fwrite (&bf, 1, sizeof (BITMAPFILEHEADER), bmp);
	fwrite (&bi, 1, sizeof (BITMAPINFOHEADER), bmp);
	fwrite (palette, 4, 256, bmp);
	for (int i = height-1; i >= 0; i--)
		fwrite (&buf[i*pitch], 1, pitch, bmp);
	fclose (bmp);

	delete [] buf;

	return true;
}


bool convert32 (char *name)
{
	FILE *cnv = fopen (name, "rb");

	int width, height, pitch;
	fseek (cnv, 1, SEEK_SET);
	fread (&width, 4, 1, cnv);
	fread (&height, 4, 1, cnv);
	fread (&pitch, 4, 1, cnv);
	fseek (cnv, 0x11, SEEK_SET);

	char *buf = new char[pitch*height*4];
	fread (buf, 1, pitch*height*4, cnv);

	fclose (cnv);

	BITMAPFILEHEADER bf = {0x4D42, (pitch*height*4)+0x36, 0, 0, 0x36};
	BITMAPINFOHEADER bi = {0x28, pitch, height, 1, 32, BI_RGB, (pitch*height*4), 0, 0, 0, 0};

	// 出力ファイル名作成
	char sbmp[128];
	strcpy (sbmp, name);
	strcpy (strrchr (sbmp, '.'), ".bmp");

	FILE *bmp = fopen (sbmp, "wb");
	if (bmp == NULL) {
		delete [] buf;
		return false;
	}

	fwrite (&bf, 1, sizeof (BITMAPFILEHEADER), bmp);
	fwrite (&bi, 1, sizeof (BITMAPINFOHEADER), bmp);
	for (int i = height-1; i >= 0; i--)
		fwrite (&buf[i*pitch*4], 1, pitch*4, bmp);
	fclose (bmp);

	delete [] buf;

	return true;
}
