#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdarg.h>
#include <vsprintf.h>

#include <InfoNES.h>
#include <InfoNES_System.h>
#include <InfoNES_pAPU.h>

#define SOUND_DEVICE "sys:/dev/pcspeaker2"

void start_application( char *filename );
int InfoNES_Load( const char *pszFileName );
void InfoNES_Main();
/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/

char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;


/* Pad state */
DWORD dwKeyPad1;
DWORD dwKeyPad2;
DWORD dwKeySystem;

int LoadSRAM();
int SaveSRAM();

/* Palette data */
WORD NesPalette[ 64 ] =
{
  0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
  0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
  0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
  0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
  0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
  0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};

/* 音频控制 */
int soundFd;
int wavflag;

/* 绘制缓冲区 */
DWORD graphBuffer[NES_DISP_WIDTH*NES_DISP_HEIGHT];

int main(int argc,char *argv[])
{
    
	//printf("hello \n");

  /*int fd = fopen("c:/nes", O_RDONLY);

	printf("hello \n");
	*/

  /*-------------------------------------------------------------------*/
  /*  Pad Control                                                      */
  /*-------------------------------------------------------------------*/

  /* Initialize a pad state */
  dwKeyPad1   = 0;
  dwKeyPad2   = 0;
  dwKeySystem = 0;
  //printf("infoNes \n");
  /* If a rom name specified, start it */
  if ( argc == 2 )
  {
    //printf("2arg %x\n", argv[ 1 ]);
    start_application( argv[ 1 ] );
  }
  //printf("arg error!\n");
  //while(1);
	return 0;	
}

/*===================================================================*/
/*                                                                   */
/*     start_application() : Start NES Hardware                      */
/*                                                                   */
/*===================================================================*/
void start_application( char *filename )
{
  //printf("load ");
  /* Set a ROM image name */
  strcpy( szRomName, filename );

  //printf("file name:%s\n", filename);
  //printf("load ");
  /* Load cassette */
  if ( InfoNES_Load ( szRomName ) == 0 )
  { 
    /* Set graphic content */
    //printf("load2 ");
    /* Load SRAM */
    LoadSRAM();
    //printf("main ");
    InfoNES_Main();

  }
}

/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
int LoadSRAM()
{
/*
 *  Load a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be read
 */

  int fd = -1;
  unsigned char pSrcBuf[ SRAM_SIZE ];
  unsigned char chData;
  unsigned char chTag;
  int nRunLen;
  int nDecoded;
  int nDecLen;
  int nIdx;

  // It doesn't need to save it
  nSRAM_SaveFlag = 0;

  // It is finished if the ROM doesn't have SRAM
  if ( !ROM_SRAM )
    return 0;

  // There is necessity to save it
  nSRAM_SaveFlag = 1;

  // The preparation of the SRAM file name
  strcpy( szSaveName, szRomName );
  strcpy( strrchr( szSaveName, '.' ) + 1, "srm" );

  /*-------------------------------------------------------------------*/
  /*  Read a SRAM data                                                 */
  /*-------------------------------------------------------------------*/

  // Open SRAM file
  fd = open( szSaveName, O_RDONLY );
  if ( fd == -1 )
    return -1;

  // Read SRAM data
  read( fd, pSrcBuf, SRAM_SIZE );

  // Close SRAM file
  close( fd );

  /*-------------------------------------------------------------------*/
  /*  Extract a SRAM data                                              */
  /*-------------------------------------------------------------------*/

  nDecoded = 0;
  nDecLen = 0;

  chTag = pSrcBuf[ nDecoded++ ];

  while ( nDecLen < 8192 )
  {
    chData = pSrcBuf[ nDecoded++ ];

    if ( chData == chTag )
    {
      chData = pSrcBuf[ nDecoded++ ];
      nRunLen = pSrcBuf[ nDecoded++ ];
      for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
      {
        SRAM[ nDecLen++ ] = chData;
      }
    }
    else
    {
      SRAM[ nDecLen++ ] = chData;
    }
  }

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           SaveSRAM() : Save a SRAM                                */
/*                                                                   */
/*===================================================================*/
int SaveSRAM()
{
/*
 *  Save a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be written
 */

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*                  InfoNES_Menu() : Menu screen                     */
/*                                                                   */
/*===================================================================*/
int InfoNES_Menu()
{
/*
 *  Menu screen
 *
 *  Return values
 *     0 : Normally
 *    -1 : Exit InfoNES
 */

  /* Nothing to do here */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*               InfoNES_ReadRom() : Read ROM image file             */
/*                                                                   */
/*===================================================================*/
int InfoNES_ReadRom( const char *pszFileName )
{
/*
 *  Read ROM image file
 *
 *  Parameters
 *    const char *pszFileName          (Read)
 *
 *  Return values
 *     0 : Normally
 *    -1 : Error
 */

  int fd;

  /* Open ROM file */
  fd = open( pszFileName, O_RDONLY );
  if ( fd == -1 )
    return -1;

  /* Read ROM Header */
  read( fd, &NesHeader, sizeof NesHeader );
  if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
  {
    /* not .nes file */
    close( fd );
    return -1;
  }

  /* Clear SRAM */
  memset( SRAM, 0, SRAM_SIZE );

  /* If trainer presents Read Triner at 0x7000-0x71ff */
  if ( NesHeader.byInfo1 & 4 )
  {
    read( fd, &SRAM[ 0x1000 ], 512);
  }

  /* Allocate Memory for ROM Image */
  ROM = (BYTE *)malloc( NesHeader.byRomSize * 0x4000 );

  /* Read ROM Image */
  read( fd, ROM, 0x4000*NesHeader.byRomSize);

  if ( NesHeader.byVRomSize > 0 )
  {
    /* Allocate Memory for VROM Image */
    VROM = (BYTE *)malloc( NesHeader.byVRomSize * 0x2000 );

    /* Read VROM Image */
    read( fd, VROM, 0x2000*NesHeader.byVRomSize );
  }

  /* File close */
  close( fd );

  /* Successful */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           InfoNES_ReleaseRom() : Release a memory for ROM         */
/*                                                                   */
/*===================================================================*/
void InfoNES_ReleaseRom()
{
/*
 *  Release a memory for ROM
 *
 */
//printf("release ");
  if ( ROM )
  {
    free( ROM );
    ROM = NULL;
  }

  if ( VROM )
  {
    free( VROM );
    VROM = NULL;
  }
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemoryCopy() : memcpy                         */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemoryCopy( void *dest, const void *src, int count )
{
/*
 *  memcpy
 *
 *  Parameters
 *    void *dest                       (Write)
 *      Points to the starting address of the copied block's destination
 *
 *    const void *src                  (Read)
 *      Points to the starting address of the block of memory to copy
 *
 *    int count                        (Read)
 *      Specifies the size, in bytes, of the block of memory to copy
 *
 *  Return values
 *    Pointer of destination
 */

  memcpy( dest, src, count );
  return dest;
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemorySet() : memset                          */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemorySet( void *dest, int c, int count )
{
/*
 *  memset
 *
 *  Parameters
 *    void *dest                       (Write)
 *      Points to the starting address of the block of memory to fill
 *
 *    int c                            (Read)
 *      Specifies the byte value with which to fill the memory block
 *
 *    int count                        (Read)
 *      Specifies the size, in bytes, of the block of memory to fill
 *
 *  Return values
 *    Pointer of destination
 */

  memset( dest, c, count);  
  return dest;
}
typedef unsigned char   guchar;

guchar  pbyRgbBuf[ NES_DISP_WIDTH * NES_DISP_HEIGHT * 3 ];

/*===================================================================*/
/*                                                                   */
/*      InfoNES_LoadFrame() :                                        */
/*           Transfer the contents of work frame on the screen       */
/*                                                                   */
/*===================================================================*/
void InfoNES_LoadFrame()
{
  //printf("f");

/*
 *  Transfer the contents of work frame on the screen
 *
 */

  //register guchar* pBuf;

  //pBuf = pbyRgbBuf;

    register DWORD *q = graphBuffer;
    static int sum = 0;
  // Exchange 16-bit to 24-bit  
  for ( register int y = 0; y < NES_DISP_HEIGHT; y++ )
  {
    for ( register int x = 0; x < NES_DISP_WIDTH; x++ )
    {  
      WORD wColor = WorkFrame[ ( y << 8 ) + x ];
	  sum += wColor;
      *q = (guchar)( ( wColor & 0x7c00 ) >> 7 )<<16;
        *q |= (guchar)( ( wColor & 0x03e0 ) >> 2 )<<8;
        *q |= (guchar)( ( wColor & 0x001f ) << 3 );
        q++;
      //*(pBuf++) = (guchar)( ( wColor & 0x7c00 ) >> 7 );
      
      //*(pBuf++) = (guchar)( ( wColor & 0x03e0 ) >> 2 );
      
      //*(pBuf++) = (guchar)( ( wColor & 0x001f ) << 3 );
    }
  }
    graph(0, (NES_DISP_WIDTH << 16) | NES_DISP_HEIGHT, graphBuffer);
    //printf("sum:%d", sum);
}

/* 
status = read(0, &key, 4);
		
		if (status != -1) {
			//获取按键
			//读取控制键状态
			
			if ((0 < key) || key&FLAG_PAD) {
				//是一般字符就传输出去
				*buf = (char)key;
				break;
			}
			
			key = -1;
		}
*/
#define SDLK_RIGHT 'D'
#define SDLK_RIGHT2 'd'

#define SDLK_LEFT 'A'
#define SDLK_LEFT2 'a'

#define SDLK_DOWN 'S'
#define SDLK_DOWN2 's'

#define SDLK_UP 'W'
#define SDLK_UP2 'w'

#define SDLK_RETURN 'N'
#define SDLK_RETURN2 'n'

#define SDLK_ESCAPE 'M'
#define SDLK_ESCAPE2 'm'

#define SDLK_J 'J'
#define SDLK_J2 'j'

#define SDLK_K 'K'
#define SDLK_K2 'k'

#define SDLK_C 'C'
#define SDLK_C2 'c'

#define SDLK_R 'R'
#define SDLK_R2 'r'

// 按键处理
void PollEvent(void)
{
    /* 轮训 */
    unsigned int key = 0;
    int status = read(0, &key, 4);
	if (status != -1) {
        key &= 0xff;
        /* 按键按下 */
		if ((0 < key) || (key&0x8000)) {
            if (key == SDLK_RIGHT || key == SDLK_RIGHT2) {
                dwKeyPad1 |= (1 << 7);
            } else {
                dwKeyPad1 &= ~(1 << 7);
            }
            if (key == SDLK_LEFT || key == SDLK_LEFT2) {
                dwKeyPad1 |= (1 << 6);
            } else {
                dwKeyPad1 &= ~(1 << 6);
            }
            
            if (key == SDLK_DOWN || key == SDLK_DOWN2) {
                dwKeyPad1 |= (1 << 5);
            } else {
                dwKeyPad1 &= ~(1 << 5);
            } 
            
            if (key == SDLK_UP || key == SDLK_UP2) {
                dwKeyPad1 |= (1 << 4);
            } else {
                dwKeyPad1 &= ~(1 << 4);
            
            } 
            if (key == SDLK_RETURN || key == SDLK_RETURN2) { /* Start */
                dwKeyPad1 |= (1 << 3);
            } else {
                dwKeyPad1 &= ~(1 << 3);
            
            }
            if (key == SDLK_ESCAPE || key == SDLK_ESCAPE2) { /* Select */
                dwKeyPad1 |= (1 << 2);
            } else {
                dwKeyPad1 &= ~(1 << 2);
            
            } 
            if (key == SDLK_J || key == SDLK_J2) { /* 'B' */
                dwKeyPad1 |= (1 << 1);
            } else {
                dwKeyPad1 &= ~(1 << 1);
            
            }    
            if (key == SDLK_K || key == SDLK_K2) { /* 'A' */
                dwKeyPad1 |= (1 << 0);
            } else {
                dwKeyPad1 &= ~(1 << 0);
            }

            if (key == SDLK_C || key == SDLK_C2) {
                /* 切换剪裁 */
				PPU_UpDown_Clip = (PPU_UpDown_Clip ? 0 : 1);
            } else if (key == SDLK_R) {
                SaveSRAM();
            }
		}
	} else {

    }
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_PadState() : Get a joypad state               */
/*                                                                   */
/*===================================================================*/
void InfoNES_PadState( DWORD *pdwKeyPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
/*
 *  Get a joypad state
 *
 *  Parameters
 *    DWORD *pdwKeyPad1                   (Write)
 *      Joypad 1 State
 *
 *    DWORD *pdwPad2                   (Write)
 *      Joypad 2 State
 *
 *    DWORD *pdwSystem                 (Write)
 *      Input for InfoNES
 *
 */
PollEvent();
  /* Transfer joypad state */
  *pdwKeyPad1   = dwKeyPad1;
  *pdwPad2   = dwKeyPad2;
  *pdwSystem = dwKeySystem;
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundInit() : Sound Emulation Initialize           */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundInit( void ) 
{
    soundFd = 0;
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundOpen() : Sound Open                           */
/*                                                                   */
/*===================================================================*/
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate ) 
{
    soundFd = open(SOUND_DEVICE, O_RDWR);
    if (soundFd < 0) {
        soundFd = 0;
        return 0;
    }
    /* 开始播放声音 */
    ioctl(soundFd, 0, 0);
  /* Successful */
  return 1;
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundClose() : Sound Close                         */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundClose( void ) 
{
  if (soundFd) {
      close(soundFd);
    }
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_SoundOutput() : Sound Output 5 Waves           */           
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundOutput( int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5 )
{
  int i;
  BYTE finalWave;
  wavflag = 1;
  if (soundFd) {
      for (i = 0; i < samples; i++) {
#if 1
    finalWave = ( wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i] ) / 5;
#else   
          /* 读取数据 */
          finalWave = wave4[i];
#endif
        /* 设置频率 */
        if (finalWave) {
            if (wavflag == 1) {
                ioctl(soundFd, 1, 0);    
            }
            wavflag++;
                
            ioctl(soundFd, 4, finalWave + 30);
        } else {
            wavflag = 0;
            ioctl(soundFd, 0, 0);
            
        }
  
      }
      /* 设置频率 */
      //ioctl(soundFd, 4, finalWave);
  }
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_Wait() : Wait Emulation if required            */
/*                                                                   */
/*===================================================================*/
void InfoNES_Wait() {}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_MessageBox() : Print System Message            */
/*                                                                   */
/*===================================================================*/
void InfoNES_MessageBox( char *pszMsg, ... )
{
  char pszErr[ 1024 ];

	va_list args = (va_list)((char*)(&pszMsg) + 4); /*4是参数fmt所占堆栈中的大小*/
	vsprintf(pszErr, pszMsg, args);

  //printf("%s", pszErr);
}

#include <InfoNES.c>
#include <InfoNES_Mapper.c>
#include <InfoNES_pAPU.c>
#include <K6502.c>
