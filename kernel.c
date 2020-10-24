#include "includes/keyboard_map.h"
#include "includes/itoa.h"
#include "includes/terminal_fg_colors.h"
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
//#define BACKSPACE_KEY_CODE 0x0E

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* current cursor location */
unsigned int current_loc = 0;
int current_loc_y = 0;
int current_loc_x = 11;
char current_command[255] = {0};
int current_command_length = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);


	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	write_port(0x21 , 0xFD);
}

void kprint(const char *str,int color)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = color;

	}
}
void back_cursor_function_x(){
	current_loc_x = current_loc_x - 1;
	//current_loc_y = current_loc_y + 1;
	update_cursor((int)current_loc_x,(int)current_loc_y);
}
void reset_cursor_function_x(){

	current_loc_x = current_loc_x - current_loc_x + 11;
	//current_loc_y = current_loc_y + 1;
	update_cursor((int)current_loc_x,(int)current_loc_y);
}
void update_cursor_function_x(){
	current_loc_x = current_loc_x + 1 ;
	//current_loc_y = current_loc_y + 1;
	update_cursor((int)current_loc_x,(int)current_loc_y);
}
void reset_cursor_function_y(){
	//current_loc_x = current_loc_x + 1 ;
	current_loc_y = current_loc_y - current_loc_y;
	update_cursor((int)current_loc_x,(int)current_loc_y);
}

void update_cursor_function_y(){
	//current_loc_x = current_loc_x + 1 ;
	current_loc_y = current_loc_y + 1;
	update_cursor((int)current_loc_x,(int)current_loc_y);
}


void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
	update_cursor_function_y();
	
}



void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
}


void enable_cursor(int cursor_start, int cursor_end)
{
	write_port(0x3D4, 0x0A);
	write_port(0x3D5, (read_port(0x3D5) & 0xC0) | cursor_start);
 
	write_port(0x3D4, 0x0B);
	write_port(0x3D5, (read_port(0x3D5) & 0xE0) | cursor_end);
}

void update_cursor(int x, int y)
{
	int pos = y * COLUMNS_IN_LINE + x;
 
	write_port(0x3D4, 0x0F);
	write_port(0x3D5, (int) (pos & 0xFF));
	write_port(0x3D4, 0x0E);
	write_port(0x3D5, (int) ((pos >> 8) & 0xFF));
}

int get_cursor_position(void)
{
    int pos = 0;
    write_port(0x3D4, 0x0F);
    pos |= read_port(0x3D5);
    write_port(0x3D4, 0x0E);
    pos |= ((int)read_port(0x3D5)) << 8;
    return pos;
}


void keyboard_handler_main(void)
{
	
	unsigned char status;
	char keycode;

	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);

	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			reset_cursor_function_x();
			current_command[current_command_length]  = '\0';
			unsigned int i = 0;
			char command[255] = "clear\0";
			int command_status = 1;
			int loop_command_length = 0;
			kprint("'",VGA_COLOR_GREEN);
			while (current_command[i] != '\0') {
				if (keyboard_map[(unsigned char) current_command[i]] == command[i]){
					if (command_status == 0){}
					else{
						command_status =1;

					}	
				}
				else{
					command_status = 0;
				}
				vidptr[current_loc++] =  keyboard_map[(unsigned char) current_command[i]];
				vidptr[current_loc++] = 0x07;
				i++;
				loop_command_length++;
			}
			if (loop_command_length == 0){kprint("Please enter a command",VGA_COLOR_GREEN);}
			else{
				kprint("'",VGA_COLOR_GREEN);
				if (command_status==1){
						clear_screen();
						reset_cursor_function_y();
						reset_cursor_function_x();
						current_loc = 0;
					

				}
				else{
					kprint(" is a Invalid Command",VGA_COLOR_GREEN);
				}
			}
			i=0;
			kprint_newline();

			current_command_length = 0;


			kprint((const char*)"TERMINAL>> ",VGA_COLOR_GREEN);
			return;
		}
		if (keyboard_map[(unsigned char) keycode] == '\b'){
			if (current_command_length <=0){

			}
			else{
				current_loc = current_loc - 2;
				vidptr[current_loc++] = ' ';
				vidptr[current_loc++] = 0x07;
				current_loc = current_loc - 2;
				current_command[current_command_length - 1] = '0';
				current_command_length = current_command_length - 1;
				back_cursor_function_x();
			}
			return;
		}
		update_cursor_function_x();
		/*
		char buffer [33];
		kprint(itoa( (int)current_loc, buffer, ITOA_DECIMAL_VALUE ),VGA_COLOR_GREEN);
		*/


		current_command[current_command_length]  = (char)keycode ;
		current_command_length++;
		
		vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
		vidptr[current_loc++] = 0x07;

	}
}

void kmain(void)
{
	


	const char *str = "Lisse Os 0.0.1 x86";
	clear_screen(); // clear all the junk on the screen before we start 
	kprint(str,VGA_COLOR_CYAN);
	

	kprint_newline();
	
	kprint_newline();
	
	

	idt_init();
	kprint("[",VGA_COLOR_GREEN);
	kprint("1",VGA_COLOR_LIGHT_BLUE);
	kprint("]",VGA_COLOR_GREEN);
	kprint(" Loaded IDT!",VGA_COLOR_GREEN);
	kprint_newline();

	kb_init();
	kprint("[",VGA_COLOR_GREEN);
	kprint("2",VGA_COLOR_LIGHT_BLUE);
	kprint("]",VGA_COLOR_GREEN);
	kprint(" Loaded keyboard support!",VGA_COLOR_GREEN);
	kprint_newline();
	
	enable_cursor(0,15);
	kprint("[",VGA_COLOR_GREEN);
	kprint("3",VGA_COLOR_LIGHT_BLUE);
	kprint("]",VGA_COLOR_GREEN);
	kprint(" Enabled Curser!",VGA_COLOR_GREEN);
	kprint_newline();
	kprint_newline();



	kprint((const char*)"TERMINAL>> ",VGA_COLOR_GREEN);

	while(1);
}
