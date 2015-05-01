//プロトタイプ
FILE *fopen2(char *fn,char *type);
void SetAppDir(void);
void ex_cv01(unsigned char *data,int len,char *fn);
void ex_cv3(unsigned char *data,int len,char *fn);
unsigned int endian(unsigned int a);
unsigned short endian16(unsigned short a);
unsigned long crc(unsigned char *buf,int len);

//グローバル変数
extern char AppDir[256];	//exeが置かれているディレクトリの絶対パス
