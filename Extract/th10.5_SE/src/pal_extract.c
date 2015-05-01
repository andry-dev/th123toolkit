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

//オブジェ指向であればprivate
FILE *fp = NULL;
//オブジェ指向であればpublic
struct data_list *list = NULL;
short fcount,pcount;
int dat_size;
int print = 1;

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
		if(print)printf("指定されたファイルが存在しません\n");
		ret = 1;
		goto end;
	}
	size = ftell(fp);
	fseek(fp,0,SEEK_END);
	size = ftell(fp)-size;
	if(size < 6){
		if(print)printf("格納数を読み取れません\nファイルサイズが小さすぎます\n");
		ret = 2;
		goto end;
	}
	fseek(fp,0,SEEK_SET);
	fread(&fcount,2,1,fp);
	fread(&flist,4,1,fp);
	if(size < flist+6){
		if(print)printf("ヘッダサイズ指定が異常です\n");
		ret = 3;
		goto end;
	}
	str = malloc(flist);
	if(str == NULL){
		if(print)printf("メモリー確保に失敗しました\n格納されたファイル数が多すぎます\n");
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
		if(print)printf("メモリー確保に失敗しました\n格納されたファイル数が多すぎます\n");
		ret = 5;
		goto free;
	}
	s = str;
	i = 0;
	last = 0;
	dat_size = 0;
	pcount = 0;
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
			if(print)printf("ヘッダ情報が不正です\n");
			ret = 6;
			goto free;
		}
		if(strcmp(&(list[i].fn[len-4]),".pal") == 0){
			pcount++;
		}
		dat_size += list[i].size;
		i++;
	}
	if(last > size){
		if(print)printf("指定アドレスがファイル終端を越えています\n");
		ret = 7;
		goto free;
	}
/*
	i = 0;
	while(i < fcount){
		if(print)printf("0x%08x %8d %s\n",list[i].addr,list[i].size,list[i].fn);
		i++;
	}
*/
	if(print)printf("%d個のファイルを検出しました。\n",pcount);

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
		if(print)printf("メモリー確保に失敗しました\n展開対象のファイルサイズが大きすぎる恐れがあります\n");
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
int main(int argc,char *argv[]){
	int i,count,temp,size,ecount;
	char *str,*fn,s[256],pal[256] = "";
	FILE *fp;
	time_t t,a,b;

	if(argc < 2)return 0;
	SetAppDir();

	i = 1;
	while(i < argc){
		if(strcmp(argv[i],"--noprint") == 0){
			print = 0;
		} else if(strcmp(&argv[i][strlen(argv[i])-4],".pal") == 0){
			strcpy(pal,argv[i]);
		}
		i++;
	}

	i = 1;
	ecount = 0;
	while(i < argc){
		if(strncmp(argv[i],"--",2) != 0 && strcmp(&argv[i][strlen(argv[i])-4],".pal") != 0){
			if(print)printf("%sをth105dat形式だと解釈し展開します。\n",argv[i]);
			if(0 != (temp = getlist(argv[i]))){
				if(print)printf("datの読み込みに失敗しました(エラー%d)\n",temp);
			} else {
				if(pcount)if(print)printf("ファイル展開を始めます。\n展開数");
				time(&t);
				size = 0;
				count = 0;
				while(count < fcount){
					fn = list[count].fn;
					size += list[count].size;
					if(strcmp(&fn[strlen(fn)-4],".pal") == 0 && (print || (strcmp(fn,pal) == 0)) ){
						ecount++;
						str = extract(count);
						if(print){
							fp = fopen2(fn,"wb");
							fwrite(str,1,list[count].size,fp);
							fclose(fp);
						} else {
							fwrite(str,1,list[count].size,stdout);
						}

						free(str);
					}
					count++;
					time(&a);
					a = a-t;
					if(size > 0)b = (dat_size-size)*a/size;
					else b = 0;
					if(print)printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%5d/%5d %02d:%02d(残り%02d:%02d)",ecount,pcount,(int)a/60,(int)a%60,(int)b/60,(int)b%60);
				}
			}
			list_free();
			if(print)printf("\n展開終了しました\n");
		}
		i++;
	}
	if(print){
		printf("全ての処理が終了しました\n終了する場合はENTERを押してください\n");
		getchar();
	}
	return 0;
}

