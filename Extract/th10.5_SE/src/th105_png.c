#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <windows.h>
#include "th105_lib.h"

typedef struct color{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char alpha;
}COLOR;

extern COLOR pal[256];
void load_pal(char *fn,int n);
int cv2_flag = 1;

int adler32(unsigned char *data,int len){
	unsigned short s1,s2;
	int i;

	i = 0;
	s1 = 1;
	s2 = 0;
	while(i < len){
		s1 = (s1 + data[i])%65521;
		s2 = (s2 + s1)%65521;
		i++;
	}
	return (s2<<16) + s1;
}

void setchunk(char *p,int len,char *name,char *data){
	*(unsigned int *)p = endian(len);
	memcpy(&p[4],name,4);
	memcpy(&p[8],data,len);
	*(unsigned int *)&p[len+8] = endian(crc(&p[4],len+4));
}

void set_compresseddata(unsigned char *data,unsigned int height,unsigned int width,char *col,int flag){
	unsigned char str[256];
	char *c;
	int i,h,w;

	if(flag)*str = 01;
	else *str = 0;
	*(unsigned short *)&str[1] = height*width+height;
	*(unsigned short *)&str[3] = ~(*(unsigned short *)&str[1]);
	memcpy(data,str,5);

	c = col;
	i = 0;
	h = 0;
	while(h < height){
		w = 0;
		data[5+i] = 0;
		i++;
		while(w < width){
			data[5+i] = *c;
			c++;
			i++;
			w++;
		}
		h++;
	}
}

void pngout(unsigned int width2,unsigned int height,unsigned int width,char *col,char *fn,int cn){
	unsigned int h,w,i,count,len,a,b,d,num;
	unsigned char str[768],s[1024],*data,*e,*c;
	unsigned int adler;
	FILE *fp;


	fp = fopen2(fn,"wb");
	*(int *)s = 0x474e5089;
	*(int *)&s[4] = 0x0a1a0a0d;
	fwrite(s,4,2,fp);

	*(unsigned int *)str = endian(width);
	*(unsigned int *)&str[4] = endian(height);
	str[8] = 8;
	if(cn == 8)str[9] = 3;
	else str[9] = 6;
	str[10] = 0;
	str[11] = 0;
	str[12] = 0;
	setchunk(s,13,"IHDR",str);
	fwrite(s,1,25,fp);

	if(width2 != width){
		*(unsigned int *)str = width2;
		setchunk(s,4,"orIx",str);
		fwrite(s,1,16,fp);
	}

	if(cn == 24 || cn == 32)width *= 4;
	if(cn == 8){
		i = 0;
		while(i < 256){
			str[i*3+0] = pal[i].r;
			str[i*3+1] = pal[i].g;
			str[i*3+2] = pal[i].b;
			i++;
		}
		setchunk(s,768,"PLTE",str);
		fwrite(s,1,780,fp);

		i = 0;
		while(i < 256){
			str[i] = pal[i].alpha;
			i++;
		}
		setchunk(s,256,"tRNS",str);
		fwrite(s,1,268,fp);
	}


	a = height*width+height;
	num = (int)ceil(a/(double)0xffff);//•ªŠ„”(len‚ª2byte‚È‚Ì‚Å)
	data = malloc(18+10*num+a);

	set_compresseddata(&data[10],height,width,col,1);
	adler = endian(adler32(&data[15],height*width+height));

	if(num == 1){
		i = 1;
	} else {
		i = 0;
		a = height;
		e = &data[10];
		c = col;
		while(1){
			d = a;
			while(d*width+d > 0xFFFF)d--;
			set_compresseddata(e,d,width,c,d == a);
			i++;
			if(d==a)break;
			c += d*width;
			e += 5+d*width+d;
			a -= d;
		}
	}

	a = height*width+height;
	len = 6+5*i+a;
	*(unsigned int *)data = endian(len);
	memcpy(&data[4],"IDAT",4);
	*(short *)&data[8] = endian16(0x7801);

	*(unsigned int *)&data[len+4] = adler;

	*(unsigned int *)&data[len+8] = endian(crc(&data[4],len+4));
	fwrite(data,1,12+len,fp);
	free(data);

	setchunk(s,0,"IEND",NULL);
	fwrite(s,1,12,fp);
	fclose(fp);
}

void ex_cv2(unsigned char *data,int len,char *fn){
	int w,h,w2,i,l;
	COLOR *col;
	char str[256],*c;
	FILE *fp;

	str[0] = data[0];
	w2 = *(unsigned int *)&data[1];
	h = *(unsigned int *)&data[5];
	w = *(unsigned int *)&data[9];
	if(*str == 24 || *str == 32){
		if(cv2_flag){
			i = 0;
			l = 0;
			col = malloc(sizeof(COLOR)*h*w2);
			while(i < h*w2){
				col[i].b = data[17+l*4];
				col[i].g = data[17+l*4+1];
				col[i].r = data[17+l*4+2];
				col[i].alpha = data[17+l*4+3];
				i++;
				l++;
				if(i%w2==0){l+=w-w2;}
			}
			pngout(w2,h,w2,(char *)col,fn,*str);
		} else {
			i = 0;
			col = malloc(sizeof(COLOR)*h*w);
			while(i < h*w){
				col[i].b = data[17+i*4];
				col[i].g = data[17+i*4+1];
				col[i].r = data[17+i*4+2];
				col[i].alpha = data[17+i*4+3];
				i++;
			}
			pngout(w2,h,w,(char *)col,fn,*str);
		}
		free(col);
	} else if(*str == 8){
		load_pal(fn,0);
		if(cv2_flag){
			i = 0;
			l = 0;
			c = malloc(h*w2);
			while(i < h*w2){
				c[i] = data[17+l];
				i++;
				l++;
				if(i%w2==0){l+=w-w2;}
			}
			pngout(w2,h,w2,c,fn,*str);
			free(c);
		} else {
			c = &data[17];
			pngout(w2,h,w,c,fn,*str);
		}
	} else {
		strcpy(str,fn);
		strcpy(&str[strlen(str)-4],".cv2");
		fp = fopen2(str,"wb");
		fwrite(data,1,len,fp);
		fclose(fp);
	}
}
