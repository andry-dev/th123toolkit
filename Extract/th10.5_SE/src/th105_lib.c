#include <stdio.h>
#include <windows.h>

char AppDir[256];

FILE *fopen2(char *fn,char *type){
	FILE *fp;
	char str[256],*s;

	fp = fopen(fn,type);
	if(fp != NULL)return fp;
	strcpy(str,fn);
	s = str;
	while(*s != '\0'){
		if(*s == '\\' || *s == '/'){
			*s = '\0';
			CreateDirectory(str,NULL);
			*s = '/';
		}
		s++;
	}
	return fopen(fn,type);
}

void SetAppDir(void){
	int i;

	GetModuleFileName(NULL,AppDir,256);
	i = strlen(AppDir)-1;
	while(AppDir[i] != '\\' && AppDir[i] != '/' && i > 0)i--;
	AppDir[i] = '\0';
	SetCurrentDirectory(AppDir);
}
void ex_cv01(unsigned char *data,int len,char *fn){
	FILE *fp;
	unsigned char a,b,c;
	char str[256];
	int i;

	fp = fopen2(fn,"wb");
	if(fp == NULL){
		printf("ファイルオープンに失敗しました(ex_cv01)\n%sは出力されません\n",fn);
		return;
	}
	a = 0x8B;
	b = 0x71;
	i = 0;
	while(i < len){
		c = data[i];
		c ^= a;
		fwrite(&c,1,1,fp);
		a += b;
		b -= 0x6B;
		i++;
	}
	fclose(fp);
}

void ex_cv3(unsigned char *data,int len,char *fn){
	FILE *fp;
	unsigned int size;
	char str[256];
	int i;

	fp = fopen2(fn,"wb");
	if(fp == NULL){
		printf("ファイルオープンに失敗しました(ex_cv3)\n%sは出力されません\n",fn);
		return;
	}
	if(!strncmp(data,"RIFF",4)){				//wav to cv3
		fwrite(&data[0x14],1,0x10,fp);
		fwrite(&data[0x28],1,len-0x28,fp);
	} else {						//cv3 to wav
		size = *(unsigned int *)&data[18];		//波形データサイズを読み込む

		strncpy(str,"RIFF",4);				//WAVEファイルヘッダを生成
		*(int *)&str[4] = size + 0x24;
		strncpy(&str[8],"WAVEfmt ",8);
		*(int *)&str[16] = 0x00000010;
		fwrite(str,1,20,fp);

		memcpy(str,data,16);				//フォーマットIDから先はCV3から取り出す
		strncpy(&str[0x10],"data",4);			//拡張フォーマット部分を置き換える
		fwrite(str,1,0x14,fp);

		fwrite(&data[18],1,len-18,fp);			//EOFまで波形データ扱いで書き込み(wav終端～EOFまで不明文字列が含まれているため)
	}
	fclose(fp);
}

unsigned int endian(unsigned int a){
	return ((unsigned char *)&a)[3] + ((unsigned char *)&a)[2]*0x100 + ((unsigned char *)&a)[1]*0x10000 + ((unsigned char *)&a)[0]*0x1000000;
}
unsigned short endian16(unsigned short a){
	return ((unsigned char *)&a)[1] + ((unsigned char *)&a)[0]*0x100;
}

unsigned long crc_table[256];
int crc_table_computed = 0;
void make_crc_table(void){
	unsigned long c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (unsigned long)n;
		for(k = 0; k < 8; k++) {
			if (c & 1)
			c = 0xedb88320L ^ (c >> 1);
			else
			c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}
unsigned long crc(unsigned char *buf,int len){
	unsigned long c = 0xffffffffL;
	int n;

	if(!crc_table_computed)make_crc_table();

	for (n = 0; n < len; n++) {
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c ^ 0xffffffffL;
}
