#include "common.h"
#include "fifo8.h"

extern char mcursor[256];

struct wjn {
    int t;
    int w;
};

struct fifo key_fifo;
struct fifo mouse_fifo;

struct screen_postion {
    int x;
    int y;
} screen_pos = {
    .x = 0,
    .y = 20,
};

struct mouse_info mouse_status = { 0 };

static void set_segmdesc( struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar )
{
    if( limit > 0xfffff )
    {
        ar |= 0x8000; /* G_bit = 1 */
        limit /= 0x1000;
    }
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
}

static void set_gatedesc( struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar )
{
    gd->offset_low   = offset & 0xffff;
    gd->selector     = selector;
    gd->dw_count     = ( ar >> 8 ) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high  = ( offset >> 16 ) & 0xffff;
}

static void init_gdtidt(void)
{
    /* Global descriptor table */
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) (ADR_GDT - (SYSSEG << 4));
    /* Interrupt descriptor table */
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) (ADR_IDT - (SYSSEG << 4));
    /* Reset GDT */
    for( int i = 0; i <= LIMIT_GDT / 8; i++ )
    {
        set_segmdesc( gdt + i, 0, 0, 0 );
    }
#if 0
    set_segmdesc( gdt + 2, 0xffffffff, 0x00010000, AR_CODE32_ER );
    set_segmdesc( gdt + 3, LIMIT_BOTPAK, ADR_BOTPAK, AR_DATA32_RW );
    load_gdtr( LIMIT_GDT, ADR_GDT );
#endif
    /* Reset IDT */
    for( int i = 0; i <= LIMIT_IDT / 8; i++ )
    {
        set_gatedesc( idt + i, 0xfff, 0, 0 );
    }
    
    load_idtr(LIMIT_IDT, ADR_IDT);

    /* Set the 21h, 27h and 2ch int handlers in the IDT*/
    set_gatedesc( idt + 0x21, (int) inthandler21, 2 * 8, AR_INTGATE32 );
    set_gatedesc( idt + 0x27, (int) inthandler27, 2 * 8, AR_INTGATE32 );
    set_gatedesc( idt + 0x2c, (int) inthandler2c, 2 * 8, AR_INTGATE32 );
}

static void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            vram[y * xsize + x] = c;
        }
    }
}

static void draw_screen()
{
    unsigned char *vram = VRAM_ADDR;
    const unsigned short xsize = VGA_X_MAX;
    const unsigned short ysize = VGA_Y_MAX;
    char buf[50];
    //unsigned int memtotal;
    /* FIXME! memman is not used, but if I comment it out
     * there is an artifact on the screen */
    struct MEMMAN *memman = (struct MEMMAN *) (MEMMAN_ADDR - (SYSSEG << 4));
    //memman_init(memman);
    //int ret;

    static char font_A[16] = {
        0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
        0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
    };
		/*init mouse global data*/
    mouse_status.mx = (xsize - 16) / 2;
    mouse_status.my = (ysize - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);

    boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

    boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
    boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
    boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);

    //memtotal=memtest(0x400000, 0xbfffffff);
    //ret = memman_free(memman, 0x00001000, 0x0009e000);
    //if (ret != 0) {
    //    putfont8_string(vram,xsize, 28, 48, COL8_FFFFFF,font.Bitmap , "Memman Free 1 Failed");
    //}
    //ret = memman_free(memman, 0x00400000, memtotal - 0x00400000);
    //if (ret != 0) {
    //    putfont8_string(vram,xsize, 28, 48, COL8_FFFFFF,font.Bitmap , "Memman Free 2 Failed");
    //}
    //sprintf(buf,"MEMORY %d MB. %dMB Heap Free.", memtotal / (1024*1024), memman_total(memman) / (1024*1024));
    putfont8_string(vram,xsize, 8, 8, COL8_FFFFFF,font.Bitmap , "Hack Week 0x13!!!");
    putfont8_string(vram,xsize, 8, 28, COL8_FFFFFF,font.Bitmap , buf);

	//draw_mouse_on_screen(&mouse_status);
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

static void wait_KBC_sendready(void)
{
    /* Wait for the keyboard hardware to be ready */
	while( io_in8( PORT_KEYSTA ) & KEYSTA_SEND_NOTREADY );
}

static void init_keyboard(void)
{
    /* Initialize the keyboard hardware */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

static void enable_mouse(void)
{
    /* Activate the mouse */
//	unsigned char status;
//	wait_KBC_sendready();
//	io_out8(PORT_KEYCMD, 0x20);
//	wait_KBC_sendready();
//	status = ((char)io_in8(PORT_KEYDAT)|0x2);
//	status &= 0xDF;
//	wait_KBC_sendready();
//	io_out8(PORT_KEYCMD, 0x60);
//	wait_KBC_sendready();
//	io_out8(PORT_KEYDAT, status);
//
//
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return; /* If successful, the keyboard controller will return ACK (0xfa) */
}

static void putfont8(char *vram, int xsize, int x, int y, char c, const unsigned char *font_bitmap)
{
    for( int i = 0; i < 16; i++ )
    {
        char* p = vram + ( y + i ) * xsize + x;
        char d = font_bitmap[i]; /* data */
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
}


void putfont8_string(unsigned char *vram, int xsize, int x, int y, char color,const unsigned char *font_bitmap, unsigned char * string)
{
    char ch ;
    int x1 = x ? x : screen_pos.x;
    int y1 = y ? y : screen_pos.y;

    for (;y1<190;y1+=30) {
        for (;x1<310;x1+=10) {
            ch = *string;
            if (ch == 0)
                break;
            putfont8(vram, xsize,  x1, y1, color, font_bitmap + (ch - 31) * 16);
            string++;

        }
        if (x == 0 && y == 0) {
            screen_pos.x += 2;
            if (screen_pos.x >= 300) {
                screen_pos.y += 12;
                screen_pos.x = 0;
            }
        }
    }
    //screen_pos.y += 1;
}

static void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    return;
}

static void init_palette(void)
{
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   /*  0:*/
        0xff, 0x00, 0x00,   /*  1:*/
        0x00, 0xff, 0x00,   /*  2:*/
        0xff, 0xff, 0x00,   /*  3:*/
        0x00, 0x00, 0xff,   /*  4:*/
        0xff, 0x00, 0xff,   /*  5:*/
        0x00, 0xff, 0xff,   /*  6:*/
        0xff, 0xff, 0xff,   /*  7:*/
        0xc6, 0xc6, 0xc6,   /*  8:*/
        0x84, 0x00, 0x00,   /*  9:*/
        0x00, 0x84, 0x00,   /* 10:*/
        0x84, 0x84, 0x00,   /* 11:*/
        0x00, 0x00, 0x84,   /* 12:*/
        0x84, 0x00, 0x84,   /* 13:*/
        0x00, 0x84, 0x84,   /* 14:*/
        0x84, 0x84, 0x84    /* 15:*/
    };
    set_palette(0, 15, table_rgb);
    return;

    /* static char <96>$<97>ß<82>Í<81>A<83>f<81>[<83>^<82>É<82>µ<82>©<8e>g<82>¦<82>È<82>¢<82>¯<82>ÇDB<96>$<97>ß<91><8a><93><96> */
}

static void init_pic(void)
{
    io_out8(PIC1_OCW1,  0xFF); 
    io_out8(PIC0_OCW1,  0xFF); 

    io_out8(PIC0_COMMAND,   0x11);
    io_out8(PIC1_COMMAND,   0x11);

    io_out8(PIC1_DATA, 0x28  );
    io_out8(PIC0_DATA, 0x20  );

    io_out8(PIC0_DATA, 1 << 2);
    io_out8(PIC1_DATA, 2);

    io_out8(PIC0_DATA, 0x01); /*8086 mode for both*/
    io_out8(PIC1_DATA, 0x01);

    io_out8(PIC0_DATA,  0xfb); 
    io_out8(PIC1_DATA,  0xff); 

    return;
}


#define PORT_KEYDAT 0x0060
static void keyboard_handler( unsigned char data )
{
    unsigned char *vram = VRAM_ADDR;
    const unsigned short xsize = VGA_X_MAX;
    const unsigned short ysize = VGA_Y_MAX;
    unsigned char out_buffer[3];
    if( data >= 0x81 || data == -1 )
    {
        return;
    }
    out_buffer[0] = scancode[data];
    if( out_buffer[0] == 0 )
    {
        const unsigned char high_nibble = ( data & 0xf0 ) >> 4;

        if( high_nibble >= 0xa && high_nibble <= 0xf )
        {
            out_buffer[0] = 'A' + high_nibble - 10; 
        }
        else
        {
            out_buffer[0] = '0' + high_nibble;
        }

        const unsigned char low_nibble = (data & 0xf);

        if( low_nibble >= 0xA && low_nibble <= 0xF )
        {
            out_buffer[1] = 'A' + low_nibble - 10; 
        }
        else if( low_nibble >= 0 && low_nibble <= 9 )
        {
            out_buffer[1] = '0' + low_nibble;
        }

        out_buffer[2]=0;

        boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
        boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
        boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

        boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
        boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
        boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
        boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
        boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
        boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

        boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
        boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
        boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
        boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);

        putfont8_string(vram,xsize, 283, 180, COL8_00FF00,font.Bitmap , out_buffer);

        return;
    }
    out_buffer[1] = 0;

    putfont8_string( vram, xsize, 0, 0, COL8_FFFFFF,font.Bitmap , out_buffer );

    if( data == 0x30 || data == 0xB0 )
    {
        putfont8_string( vram, xsize, 8, 100, COL8_FFFFFF, font.Bitmap, ( unsigned char* )"May the source be with you!" );
    }
}

void _inthandler21(int *esp)
{
    unsigned char data;
    io_out8( PIC0_COMMAND, 0x61 );
    data = io_in8( PORT_KEYDAT );
	fifo_put( &key_fifo, data );
}

static void mouse_handler(unsigned char data)
{
    unsigned char *vram = VRAM_ADDR;
    const unsigned short xsize = VGA_X_MAX;
    const unsigned short ysize = VGA_Y_MAX;
	char buffer[20];
	switch( mouse_status.phase )
    {
    case 0:
		if( data == 0xfa )
        {
            mouse_status.phase = 1;
		}
		break;
    case 1:
		if( ( data & 0xc8 ) == 0x08 )
        {
			mouse_status.buf[0] = data;
			mouse_status.phase = 2;
		}
		break;
    case 2:
		mouse_status.buf[1] = data;
		mouse_status.phase = 3;
        break;
    case 3:
		mouse_status.buf[2] = data;
		mouse_status.phase = 1;
		mouse_status.btn = mouse_status.buf[0] & 0x07;
		mouse_status.x = mouse_status.buf[1];
		mouse_status.y = mouse_status.buf[2];
			
		if( ( mouse_status.buf[0] & 0x10 ) != 0 )
        {
			mouse_status.x |= 0xffffff00;
		}
		if( ( mouse_status.buf[0] & 0x20 ) != 0 )
        {
			mouse_status.y |= 0xffffff00;
		}
		mouse_status.y = -mouse_status.y;
		boxfill8(VRAM_ADDR, xsize, COL8_008484, mouse_status.mx, mouse_status.my, mouse_status.mx + 15, mouse_status.my + 15);
		//mouse_status.mx += mouse_status.x;
		mouse_status.mx += mouse_status.x;
		mouse_status.my += mouse_status.y;
		if( mouse_status.mx < 0 )
        {
			mouse_status.mx = 0;
		}
		if( mouse_status.my < 0 )
        {
			mouse_status.my = 0;
		}
		if( mouse_status.mx > xsize - 16 )
        {
			mouse_status.mx = xsize - 16;
		}
		if( mouse_status.my > ysize - 16 )
        {
			mouse_status.my = ysize - 16;
		}
		draw_mouse_on_screen(&mouse_status);
    default:
        break;
    }            
}
void _inthandler2c(int *esp)
{   
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);
    data = io_in8(PORT_KEYDAT);
    fifo_put(&mouse_fifo, data);
}

void _inthandler27(int *esp)
{
    io_out8(PIC0_COMMAND, 0x67); /* IRQ-07<8e>ó<95>t<8a>®<97>¹<82>ðPIC<82>É<92>Ê<92>m(7-1<8e>Q<8f>Æ) */
}

void kernelstart(char *arg)
{
    unsigned char keybuffer[32];
    unsigned char mousebuffer[128];
 
    init_palette();

    init_gdtidt();
    init_pic();
    fifo_init( &key_fifo, sizeof( keybuffer ), keybuffer );
	fifo_init( &mouse_fifo, sizeof( mousebuffer ) , mousebuffer );
    io_sti();

    io_out8(PIC0_DATA, 0xf9); /* PIC0<82>Æ<83>L<81>[<83>{<81>[<83>h<82>ð<8b><96><89>Â(11111001) */
    io_out8(PIC1_DATA, 0xef); /* <83>}<83>E<83>X<82>ð<8b><96><89>Â(11101111) */

	init_keyboard();

	enable_mouse();

    draw_screen();

	//draw_mouse_on_screen();
    while( 1 ) /* Interruptions handler loop */
    {
		io_cli(); /* Disable ints */
        if( fifo_status( &key_fifo ) > 0 ) /* If there are keyboard ints */
        {
            unsigned char data = fifo_get( &key_fifo );
            io_sti();
			keyboard_handler( data );
		}
        else if( fifo_status( &mouse_fifo ) > 0 ) /* If there are mouse ints */
        {
            unsigned char data = fifo_get( &mouse_fifo );
			io_sti();
			mouse_handler( data );
		}
        else /* If all keyboard and mouse ints are processed */
        {
            io_stihlt(); /* Enable ints and wait until next int appears */
        }
	}
}
