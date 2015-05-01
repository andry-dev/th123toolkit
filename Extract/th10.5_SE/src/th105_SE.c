#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "th105_lib.h"

struct data_list{
	int addr;
	int size;
	char *fn;
};

//本来はやってはいけない
#include "th105_lib.c"
#include "mt.c"
#include "th105_png.c"

//オブジェ指向であればprivate
FILE *fp = NULL;
//オブジェ指向であればpublic
struct data_list *list = NULL;
short fcount;
int dat_size;
COLOR pal[256];
char pal_name[256] = "";
char dat_name[256] = "";

void list_free(void){
	int i = 0;
	if(fp != NULL){
		fclose(fp);
		fp = NULL;
	}
	if(list == NULL)return;
	while(i < fcount){
		free(list[i].fn);
		i++;
	}
	free(list);
}


/*
datのヘッダ解析および展開前準備関数
終了処理の関係上gotoを使っているが、アルゴリズム見直しのおかげでendの必要性が無くなっている。
*/
int getlist(char *filename){
	int flist,size,last;
	char *str,*s;
	int i,k,t,ret;
	unsigned char len;

	list_free();
	ret = 0;
	fp = fopen(filename,"rb");
	if(fp == NULL){
		printf("指定されたファイルが存在しません\n");
		ret = 1;
		goto end;
	}
	size = ftell(fp);
	fseek(fp,0,SEEK_END);
	size = ftell(fp)-size;
	if(size < 6){
		printf("格納数を読み取れません\nファイルサイズが小さすぎます\n");
		ret = 2;
		goto end;
	}
	fseek(fp,0,SEEK_SET);
	fread(&fcount,2,1,fp);
	fread(&flist,4,1,fp);
	if(size < flist+6){
		printf("ヘッダサイズ指定が異常です\n");
		ret = 3;
		goto end;
	}
	str = malloc(flist);
	if(str == NULL){
		printf("メモリー確保に失敗しました\n格納されたファイル数が多すぎます\n");
		ret = 4;
		goto end;
	}
	fread(str,1,flist,fp);

	init_genrand(6+flist);
	i = 0;
	while(i < flist){
		str[i] ^= genrand_int32() & 0xff;
		i++;
	}
	i = 0;
	k = 0xC5;
	t = 0x83;
	while(i < flist){
		str[i] ^= k;
		k += t;
		t += 0x53;
		i++;
	}

	list = malloc(sizeof(struct data_list)*fcount);
	if(list == NULL){
		printf("メモリー確保に失敗しました\n格納されたファイル数が多すぎます\n");
		ret = 5;
		goto free;
	}
	s = str;
	i = 0;
	last = 0;
	dat_size = 0;
	while(i < fcount){
		list[i].addr = *(int *)s;
		s += 4;
		list[i].size = *(int *)s;
		s += 4;
		if(list[i].addr+list[i].size > last){
			last = list[i].addr+list[i].size;
		}
		len = *s;
		s++;
		list[i].fn = malloc(len+1);
		memcpy(list[i].fn,s,len);
		s += len;
		list[i].fn[len] = '\0';
		if(strlen(list[i].fn) != len){
			printf("ヘッダ情報が不正です\n");
			ret = 6;
			goto free;
		}
		dat_size += list[i].size;
		i++;
	}
	if(last > size){
		printf("指定アドレスがファイル終端を越えています\n");
		ret = 7;
		goto free;
	}
/*
	i = 0;
	while(i < fcount){
		printf("0x%08x %8d %s\n",list[i].addr,list[i].size,list[i].fn);
		i++;
	}
*/
	printf("%d個のファイルを検出しました。\n",fcount);
	strcpy(dat_name,filename);

free:
	free(str);
end:
	return ret;
}

char *extract(int num){
	char *str;
	int i,k;

	fseek(fp,list[num].addr,SEEK_SET);
	str = malloc(list[num].size);
	if(str == NULL){
		printf("メモリー確保に失敗しました\n展開対象のファイルサイズが大きすぎる恐れがあります\n");
		return NULL;
	}
	fread(str,1,list[num].size,fp);
	i = 0;
	k = (list[num].addr>>1) | 0x23;
	while(i < list[num].size){
		str[i] ^= k;
		i++;
	}
	return str;
}

char *pal_extract(char *filename,char *pal){
	char str[256],fn[256],*data;
	FILE *fp;
	int i,a;
	HANDLE hSearch;
	WIN32_FIND_DATA fd;

	strcpy(fn,filename);
	i = strlen(fn)-1;
	while(i > 0 && fn[i] != '\\' && fn[i] != '/')i--;
	fn[i] = '\0';
	sprintf(str,"%s/*.dat",fn);

	data = malloc(768);
	data[0] = 0;
	hSearch = FindFirstFile(str, &fd );
	if( hSearch == INVALID_HANDLE_VALUE )return data;
	while(1){
		sprintf(str,"pal_extract --noprint %s %s/%s",pal,fn,fd.cFileName);
		fp = _popen(str,"rb");
		a = fread(data,1,768,fp);
		_pclose(fp);
		if(a == 768){
			break;
		}
		if(!FindNextFile( hSearch, &fd )){
			if( GetLastError() == ERROR_NO_MORE_FILES ){
				break;
			}
		}
	}
	FindClose( hSearch );
	return data;
}

void load_pal(char *fn,int n){//th105_png.c用
	char str[256],s[15],*data;
	int i;
	FILE *fp;

	if(n > 999 || n < 0)return;

	sprintf(s,"palette%03d.pal",n);
	strcpy(str,fn);
	i = strlen(str)-1;
	while(i >= 0 && str[i] != '\\' && str[i] != '/')i--;
	strcpy(&str[i+1],s);
	if(!strcmp(str,pal_name)){
		return;
	}
	strcpy(pal_name,str);

	i = 0;
	while(i < fcount){
		if(!strcmp(str,list[i].fn)){
			break;
		}
		i++;
	}
	if(i == fcount){
		fp = fopen(str,"rb");
		if(fp == NULL){
			data = pal_extract(dat_name,str);
		} else {
			data = malloc(768);
			fread(data,1,768,fp);
			fclose(fp);
		}
	} else {
		data = extract(i);
	}
	if(data == NULL){
		return;
	} else if(*data != 0x10){
		printf("\n不正なpalファイルです\n");
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
	int i,count,temp,size;
	char *str,*fn,s[256];
	FILE *fp;
	time_t t,a,b;

	if(argc < 2)return 0;
	SetAppDir();

	i = 1;
	while(i < argc){
		printf("%sをth105dat形式だと解釈し展開します。\n",argv[i]);
		if(0 != (temp = getlist(argv[i]))){
			printf("datの読み込みに失敗しました(エラー%d)\n",temp);
		} else {
			if(fcount)printf("ファイル展開を始めます。\n展開数");
			time(&t);
			size = 0;
			count = 0;
			while(count < fcount){
				str = extract(count);
				fn = list[count].fn;
				size += list[count].size;
				
				if(!strcmp(&fn[strlen(fn)-4],".cv0")){
					strcpy(s,fn);
					strcpy(&s[strlen(s)-4],".txt");
					ex_cv01(str,list[count].size,s);
				} else if(!strcmp(&fn[strlen(fn)-4],".cv1")){
					strcpy(s,fn);
					strcpy(&s[strlen(s)-4],".csv");
					ex_cv01(str,list[count].size,s);
				} else if(!strcmp(&fn[strlen(fn)-4],".cv2")){
					strcpy(s,fn);
					strcpy(&s[strlen(s)-4],".png");
					ex_cv2(str,list[count].size,s);
				} else if(!strcmp(&fn[strlen(fn)-4],".cv3")){
					strcpy(s,fn);
					strcpy(&s[strlen(s)-4],".wav");
					ex_cv3(str,list[count].size,s);
				} else {
					fp = fopen2(fn,"wb");
					fwrite(str,1,list[count].size,fp);
					fclose(fp);
				}
				free(str);
				count++;
				time(&a);
				a = a-t;
				b = (dat_size-size)*a/size;
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%5d/%5d %02d:%02d(残り%02d:%02d)",count,fcount,(int)a/60,(int)a%60,(int)b/60,(int)b%60);
			}
		}
		list_free();
		printf("\n展開終了しました\n");
		i++;
	}
	printf("全ての処理が終了しました\n終了する場合はENTERを押してください\n");
	getchar();
	return 0;
}

