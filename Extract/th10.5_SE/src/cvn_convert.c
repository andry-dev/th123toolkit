#include <stdio.h>
#include <windows.h>
#include "th105_lib.h"

//本来やってはいけない
#include "th105_lib.c"
#include "th105_png.c"

int fcount=0;
COLOR pal[256];
char pal_name[256];

void convert(char *fn){
	FILE *fp;
	char str[256],*data,s[256],dir[256];
	int i,len,a,n;
	void (*func[4])(unsigned char*,int,char*) = {ex_cv01,ex_cv01,ex_cv2,ex_cv3};

	i = strlen(fn)-1;
	while(i >= 0 && fn[i]!='\\' && fn[i]!='/')i--;
	i++;
	strcpy(dir,fn);
	dir[i] = '\0';
	strcpy(str,&fn[i]);
	if(!strcmp(&str[strlen(str)-4],".cv0")){
		strcpy(&str[strlen(str)-4],".txt");
		n = 0;
	} else if(!strcmp(&str[strlen(str)-4],".cv1")){
		strcpy(&str[strlen(str)-4],".csv");
		n = 1;
	} else if(!strcmp(&str[strlen(str)-4],".cv2")){
		strcpy(&str[strlen(str)-4],".png");
		n = 2;
	} else if(!strcmp(&str[strlen(str)-4],".cv3")){
		strcpy(&str[strlen(str)-4],".wav");
		n = 3;
	} else if(!strcmp(&str[strlen(str)-4],".txt")){
		strcpy(&str[strlen(str)-4],".cv0");
		n = 0;
	} else if(!strcmp(&str[strlen(str)-4],".csv")){
		strcpy(&str[strlen(str)-4],".cv1");
		n = 1;
	} else if(!strcmp(&str[strlen(str)-4],".png")){
		return;
		strcpy(&str[strlen(str)-4],".cv2");
		n = 2;
	} else if(!strcmp(&str[strlen(str)-4],".wav")){
		strcpy(&str[strlen(str)-4],".cv3");
		n = 3;
	} else {
		return;
	}

	fp = fopen(fn,"rb");
	len = ftell(fp);
	fseek(fp,0,SEEK_END);
	len = ftell(fp) - len;
	fseek(fp,0,SEEK_SET);
	data = malloc(len);
	fread(data,1,len,fp);
	fclose(fp);

	sprintf(s,"%s/%s",AppDir,str);
	i = 0;
	while(NULL != (fp = fopen(s,"r")) ){
		fclose(fp);
		if(i == 0){
			a = strlen(str);
			memcpy(&str[a],&str[a-4],4);
		}
		sprintf(s,"%04d",i+1);
		memcpy(&str[a-4],s,4);
		sprintf(s,"%s/%s",AppDir,str);
		i++;
	}

	SetCurrentDirectory(dir);
	func[n](data,len,s);
	fcount++;
	printf("\b\b\b\b\b\b\b(%5d)",fcount);
}

void gl(char *fn){
	HANDLE hSearch;
	WIN32_FIND_DATA fd;

	if(!SetCurrentDirectory(fn)){
		convert(fn);
		return;
	}

	hSearch = FindFirstFile("*", &fd );
	if( hSearch == INVALID_HANDLE_VALUE )return;
	while(1){
		if(fd.cFileName[0] != '.'){
			gl(fd.cFileName);
		}
		if(!FindNextFile( hSearch, &fd )){
			if( GetLastError() == ERROR_NO_MORE_FILES ){
				break;
			}
		}
	}
	FindClose(hSearch);
	SetCurrentDirectory("../");
}

void load_pal(char *fn,int n){//th105_png.c用
	char str[256],s[15],*data;
	int i;
	FILE *fp;

	if(n > 999 || n < 0)return;

	sprintf(str,"palette%03d.pal",n);
	if(!strcmp(str,pal_name)){
		return;
	}

	fp = fopen(str,"rb");
	if(fp == NULL){
		printf("%sファイルが見つかりません\n",str);
		return;
	}
	data = malloc(768);
	fread(data,1,768,fp);
	fclose(fp);
	if(*data != 0x10){
		printf("不正なpalファイルです\n");
	} else {
		i = 0;
		while(i < 256){
			pal[i].b = (data[i*2+1]&0x1F) << 3;
			pal[i].g = ((data[i*2+1]&0xE0) >> 2) + ((data[i*2+2]&0x03) << 6);
			pal[i].r = (data[i*2+2]&0x78) << 1;
			pal[i].alpha = ((data[i*2+2]&0x80)>>7)*0xFF;
			i++;
		}
	}
	free(data);
}

int main(int argc,char *argv[]){
	int i;

	printf("対象データ変換中(    0)");
	i = 1;
	while(i < argc){
		SetAppDir();
		gl(argv[i]);
		i++;
	}

	printf("\b\b\b\b\b\b\b       \n変換完了\n対象データ数:%d\n",fcount);
	printf("終了する際はENTERを押してください。\n");
	getchar();
	return 0;
}
