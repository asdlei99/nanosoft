//
// Простой программатор микроконтроллеров AVR для Raspberry Pi
// для программирования используются стандартные выводы шины GPIO
// (MOSI - 19, MISO - 21, SCK - 23) + 15й пин для подачи сигнала RESET
//
// Written on Raspberry Pi for Raspberry Pi
//
// (c) Alex V. Zolotov <zolotov-alex@shamangrad.net>, 2013
//    Fill free to copy, to compile, to use, to redistribute and etc on your own risk.
//

//#include <bcm2835.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include <unistd.h>  /* Объявления стандартных функций UNIX */
#include <fcntl.h>   /* Объявления управления файлами */
#include <errno.h>   /* Объявления кодов ошибок */
#include <termios.h> /* Объявления управления POSIX-терминалом */

enum {
	AT_ACT_INFO,
	AT_ACT_CHECK,
	AT_ACT_WRITE,
	AT_ACT_ERASE,
	AT_ACT_READ_FUSE,
	AT_ACT_WRITE_FUSE_LO,
	AT_ACT_WRITE_FUSE_HI,
	AT_ACT_WRITE_FUSE_EX,
	AT_ACT_ADC
};

#define AT_PB3   RPI_V2_GPIO_P1_11
#define AT_PB4   RPI_V2_GPIO_P1_13

#define AT_PWR   RPI_V2_GPIO_P1_17
#define AT_RESET RPI_V2_GPIO_P1_18
#define AT_MOSI  RPI_V2_GPIO_P1_19
#define AT_MISO  RPI_V2_GPIO_P1_21
#define AT_SCK   RPI_V2_GPIO_P1_23
#define AT_GND   RPI_V2_GPIO_P1_25

#define GPIO_OUT(pin) bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP)
#define GPIO_INP(pin) bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT)

#define PACKET_MAXLEN 6

typedef struct
{
	unsigned char cmd;
	unsigned char len;
	unsigned char data[PACKET_MAXLEN];
} packet_t;

/**
* Действие которое надо выполнить
*/
int action;

/**
* Имя hex-файла
*/
const char *fname;

unsigned int fuse_bits;

/**
* Нужно ли выводить дополнительный отладочный вывод или быть тихим?
*/
int verbose = 0;

int serial_fd = 0;

int serial_init()
{
	serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd == -1)
	{
		perror("serial_init() Unable to open /dev/ttyUSB0");
		return 0;
	}
	
	struct termios options;

	// Получение текущих опций для порта...
	tcgetattr(serial_fd, &options);
	
	speed_t speed = B9600;
	
	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	
	options.c_cflag &= ~CRTSCTS;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
	options.c_iflag = 0;
	//options.c_iflag &= ~(IXON | IXOFF | IXANY);
	//options.c_iflag &= ~(ICRNL | INLCR | IGNCR);
	
	options.c_oflag = 0;
	//options.c_oflag &= ~OPOST;
	//options.c_oflag &= ~(ICRNL | INLCR | IGNCR);
	
	// Установка новых опций для порта...
	tcsetattr(serial_fd, TCSANOW, &options);
	
	fcntl(serial_fd, F_SETFL, 0);
	
	return 1;
}

int send_packet(const packet_t *pkt)
{
	ssize_t r = write(serial_fd, pkt, pkt->len + 2);
	if ( r == pkt->len + 2 ) return 1;
	else
	{
		// TODO обработка ошибок
		fprintf(stderr, "fail to send packet\n");
		return 0;
	}
}

int recv_packet(packet_t *pkt)
{
	ssize_t r;
	//printf("recv_packet begin\n");
	r = read(serial_fd, &pkt->cmd, sizeof(pkt->cmd));
	if ( r != sizeof(pkt->cmd) )
	{ /*fprintf(stderr, "cmd recv fail, r: %d\n", r);*/ return 0; }
// 	printf("cmd: %d\n", pkt->cmd);
	
	r = read(serial_fd, &pkt->len, sizeof(pkt->len));
	if ( r != sizeof(pkt->len) )
	{ /*fprintf(stderr, "len recv fail, r: %d\n", r);*/ return 0; }
// 	printf("len: %d\n", pkt->len);
	
	int i;
	for(i = 0; i < pkt->len; i++)
	{
		r = read(serial_fd, &pkt->data[i], sizeof(pkt->data[i]));
		if ( r != sizeof(pkt->data[i]) )
		{ /*fprintf(stderr, "data[%d] recv fail, r: %d\n", i, r);*/ return 0; }
// 		printf("data[%d] = %02X\n", i, pkt->data[i]);
	}
	
	return 1;
}

int cmd_seta(char value)
{
	packet_t pkt;
	pkt.cmd = 1;
	pkt.len = 1;
	pkt.data[0] = value;
	return send_packet(&pkt);
}

int cmd_isp_reset(char value)
{
	packet_t pkt;
	pkt.cmd = 2;
	pkt.len = 1;
	pkt.data[0] = value;
	return send_packet(&pkt);
}

unsigned int cmd_isp_io(unsigned int cmd)
{
	packet_t pkt;
	pkt.cmd = 3;
	pkt.len = 4;
	
	int i;
	for(i = 0; i < 4; i++)
	{
		unsigned char byte = cmd >> 24;
		pkt.data[i] = byte;
		cmd <<= 8;
	}
	
	send_packet(&pkt);
	int r = recv_packet(&pkt);
	if ( !r ) fprintf(stderr, "recv_packet failed\n");
	if ( pkt.cmd != 3 ) fprintf(stderr, "wrong packet, cmd: %d\n", pkt.cmd);
	if ( pkt.len != 4 ) fprintf(stderr, "wrong packet, len: %d\n", pkt.len);
	
	unsigned int result = 0;
	for(i = 0; i < 4; i++)
	{
		unsigned char byte = pkt.data[i];
		result = result * 256 + byte;
	}
	
	return result;
}

int cmd_adc(uint8_t admux, uint16_t *value)
{
	packet_t pkt;
	pkt.cmd = 4;
	pkt.len = 1;
	pkt.data[0] = admux;
	if ( ! send_packet(&pkt) )
	{
		fprintf(stderr, "send_packet fault (ADC)\n");
		return 0;
	}
	
	int r = recv_packet(&pkt);
	if ( !r ) fprintf(stderr, "recv_packet failed (ADC)\n");
	if ( pkt.cmd != 4 ) fprintf(stderr, "wrong packet (ADC), cmd: %d\n", pkt.cmd);
	if ( pkt.len != 2 ) fprintf(stderr, "wrong packet (ADC), len: %d\n", pkt.len);
	*value = pkt.data[1] * 256 + pkt.data[0];
	return 1;
}

/**
* Отправить комманду микроконтроллеру и прочтать ответ
* Все комманды размером 4 байта
*/
unsigned int at_io(unsigned int cmd)
{
	return cmd_isp_io(cmd);
}

/**
* Подать сигнал RESET и командду "Programming Enable"
*/
int at_program_enable()
{
	cmd_isp_reset(0);
	cmd_isp_reset(1);
	cmd_isp_reset(0);
	
	unsigned int r = cmd_isp_io(0xAC53000F);
	int status = (r & 0xFF00) == 0x5300;
	if ( verbose )
	{
		const char *s = status ? "ok" : "fault";
		printf("at_program_enable(): %s\n", s);
	}
	return status;
}

/**
* Прочитать байт прошивки из устройства
*/
unsigned char at_read_memory(unsigned int addr)
{
	unsigned int cmd = (addr & 1) ? 0x28 : 0x20;
	unsigned int offset = (addr >> 1) & 0xFFFF;
	unsigned int byte = at_io( (cmd << 24) | (offset << 8 ) ) & 0xFF;
	return byte;
}

static active_page = 0;
static dirty_page = 0;

/**
* Записать байт прошивки в устройство
* NOTE: байт сначала записывается в специальный буфер, фиксация
* данных происходит в функции at_flush(). Функция at_write_memory()
* сама переодически вызывает at_flush() поэтому нет необходимости
* часто вызывать at_flush() и необходимо только в конце, чтобы
* убедиться что последние данные будут записаны в устройство
*/
int at_write_memory(unsigned int addr, unsigned char byte)
{
	unsigned int cmd = (addr & 1) ? 0x48 : 0x40;
	unsigned int offset = (addr >> 1) & 0xFFFF;
	unsigned int page = (addr >> 1) & 0xFFF0;
	unsigned int x = 0;
	if ( page != active_page )
	{
		at_flush();
		active_page = page;
	}
	dirty_page = 1;
	unsigned int result = at_io(x = (cmd << 24) | (offset << 8 ) | (byte & 0xFF) );
	unsigned int r = (result >> 16) & 0xFF;
	int status = (r == cmd);
	if ( verbose )
	{
		printf(status ? "." : "*");
		//printf("[%04X]=%02X%s ", offset, byte, (status ? "+" : "-"));
	}
	return status;
}

/**
* Завершить запись данных
*/
int at_flush()
{
	if ( ! dirty_page )
	{
		return 1;
	}
	
	unsigned int cmd = 0x4C;
	unsigned int offset = active_page & 0xFFF0;
	unsigned int x = 0;
	unsigned int result = at_io(x = (cmd << 24) | (offset << 8 ) );
	unsigned int r = (result >> 16) & 0xFF;
	int status = (r == cmd);
	dirty_page = 0;
	if ( verbose )
	{
		printf("FLUSH[%04X]%s\n", offset, (status ? "+" : "-"));
	}
	return status;
}

/**
* выдать сообщение об ошибке в hex-файле
*/
void at_wrong_file()
{
	printf("wrong hex-file\n");
}

/**
* Конверировать шестнадцатеричную цифру в число
*/
unsigned char at_hex_digit(char ch)
{
	if ( ch >= '0' && ch <= '9' ) return ch - '0';
	if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	// TODO somethink...
	return 0;
}

unsigned int at_hex_to_int(const char *s)
{
	unsigned int r = 0;
	while ( *s )
	{
		char ch = *s++;
		unsigned int hex = 0x10;
		if ( ch >= '0' && ch <= '9' ) hex = ch - '0';
		else if ( ch >= 'A' && ch <= 'F' ) hex = ch - 'A' + 10;
		else if ( ch >= 'a' && ch <= 'f' ) hex = ch - 'a' + 10;
		else hex = 0x10;
		if ( hex > 0xF ) return r;
		r = r * 0x10 + hex;
	}
	return r;
}

/**
* Прочитать байт
*/
unsigned char at_hex_get_byte(const char *line, int i)
{
	int offset = i * 2 + 1;
	// TODO index limit checks
	return at_hex_digit(line[offset]) * 16 + at_hex_digit(line[offset+1]);
}

/**
* Прочитать слово (два байта)
*/
unsigned int at_hex_get_word(const char *line, int i)
{
	return at_hex_get_byte(line, i) * 256 + at_hex_get_byte(line, i+1);
}

/**
* Прочитать байт данных (читает из секции данных)
*/
unsigned char at_hex_get_data(const char *line, int i)
{
	return at_hex_get_byte(line, i + 4);
}

/**
* Сверить прошивку с данными из файла
*/
int at_check_firmware(const char *fname)
{
	FILE *f = fopen(fname, "r");
	if ( f )
	{
		int lineno = 0;
		int result = 1;
		int bytes = 0;
		while ( 1 )
		{
			char line[1024];
			const char *s = fgets(line, sizeof(line), f);
			if ( s == NULL ) break;
			lineno++;
			if ( line[0] != ':' )
			{
				at_wrong_file();
				break;
			}
			unsigned char len = at_hex_get_byte(line, 0);
			unsigned int addr = at_hex_get_word(line, 1);
			unsigned char type = at_hex_get_byte(line, 3);
			unsigned char cc = at_hex_get_byte(line, 4 + len);
			if ( verbose ) printf("[%04X]", addr);
			if ( type == 0 )
			{
				int i;
				for(i = 0; i < len; i++)
				{
					bytes ++;
					unsigned char fbyte = at_hex_get_data(line, i);
					unsigned char dbyte = at_read_memory(addr + i);
					int r = (fbyte == dbyte);
					result = result && r;
					if ( verbose )
					{
						printf(r ? "." : "*");
					}
				}
				if ( verbose ) printf("\n");
			}
			if ( type == 1 )
			{
				if ( verbose ) printf("end of hex-file\n");
				break;
			}
		}
		fclose(f);
		return result;
	}
	return 0;
}

/**
* Стереть чип
*/
int at_chip_erase()
{
	if ( verbose )
	{
		printf("erase device's firmware\n");
	}
	unsigned int r = at_io(0xAC800000);
	return ((r >> 16) & 0xFF) == 0xAC;
}

/**
* Записать прошивку в устройство
*/
int at_write_firmware(const char *fname)
{
	FILE *f = fopen(fname, "r");
	if ( f )
	{
		int lineno = 0;
		int result = 1;
		int bytes = 0;
		while ( 1 )
		{
			char line[1024];
			const char *s = fgets(line, sizeof(line), f);
			if ( s == NULL ) break;
			lineno++;
			if ( line[0] != ':' )
			{
				at_wrong_file();
				break;
			}
			unsigned char len = at_hex_get_byte(line, 0);
			unsigned int addr = at_hex_get_word(line, 1);
			unsigned char type = at_hex_get_byte(line, 3);
			unsigned char cc = at_hex_get_byte(line, 4 + len);
			if ( type == 0 )
			{
				int i;
				for(i = 0; i < len; i++)
				{
					bytes++;
					unsigned char fbyte = at_hex_get_data(line, i);
					int r = at_write_memory(addr + i, fbyte);
					result = result && r;
				}
			}
			if ( type == 1 )
			{
				at_flush();
				if ( verbose ) printf("end of hex-file\n");
				break;
			}
		}
		at_flush();
		fclose(f);
		if ( verbose )
		{
			char *st = result ? "ok" : "fail";
			printf("memory write: %s, bytes: %d\n", st, bytes);
		}
		return result;
	}
	return 0;
}

/**
* Действие - сверить прошивку в устрействе с файлом
*/
int at_act_check()
{
	int r = at_check_firmware(fname);
	if ( r ) printf("firmware is same\n");
	else printf("firmware differ\n");
	return r;
}

/**
* Действие - записать прошивку в устройство
*/
int at_act_write()
{
	int r = at_chip_erase();
	if ( r )
	{
		r = at_write_firmware(fname);
	}
	else
	{
		fprintf(stderr, "firmware erase fault\n");
	}
	printf("firmware write: %s\n", (r ? "ok" : "fail"));
	return r;
}

/**
* Действие - стереть чип
*/
int at_act_erase()
{
	return at_chip_erase();
}

/**
* Действие - прочитать биты fuse
*/
int at_act_read_fuse()
{
	if ( verbose )
	{
		printf("read device's fuses\n");
	}
	unsigned int fuse_lo;
	unsigned int fuse_hi;
	unsigned int fuse_ex;
	
	fuse_lo = at_io(0x50000000);
	fuse_hi = at_io(0x58080000);
	fuse_ex = at_io(0x50080000);
	
	printf("fuse[lo]: %02X (%08X)\n", (fuse_lo % 0x100), fuse_lo);
	printf("fuse[hi]: %02X (%08X)\n", (fuse_hi % 0x100), fuse_hi);
	printf("fuse[ex]: %02X (%08X)\n", (fuse_ex % 0x100), fuse_ex);
	
	return 1;
}

/**
* Действие - записать младшие биты fuse
*/
int at_act_write_fuse_lo()
{
	if ( verbose )
	{
		printf("write device's low fuse bits (0x%02X)\n", fuse_bits);
	}
	if ( fuse_bits > 0xFF )
	{
		printf("wrong fuse bits\n");
		return 0;
	}
	
	unsigned int r = at_io(0xACA00000 + (fuse_bits & 0xFF));
	int ok = ((r >> 16) & 0xFF) == 0xAC;
	if ( verbose )
	{
		const char *status = ok ? "[ ok ]" : "[ fail ]";
		printf("write device's low fuse bits %s\n", status);
	}
	
	return ok;
}

/**
* Действие - записать старшие биты fuse
*/
int at_act_write_fuse_hi()
{
	if ( verbose )
	{
		printf("write device's high fuse bits (0x%02X)\n", fuse_bits);
	}
	if ( fuse_bits > 0xFF )
	{
		printf("wrong fuse bits\n");
		return 0;
	}
	
	unsigned int r = at_io(0xACA80000 + (fuse_bits & 0xFF));
	int ok = ((r >> 16) & 0xFF) == 0xAC;
	if ( verbose )
	{
		const char *status = ok ? "[ ok ]" : "[ fail ]";
		printf("write device's high fuse bits %s\n", status);
	}
	
	return ok;
}

/**
* Действие - записать расширеные биты fuse
*/
int at_act_write_fuse_ex()
{
	if ( verbose )
	{
		printf("write device's extended fuse bits (0x%02X)\n", fuse_bits);
	}
	if ( fuse_bits > 0xFF )
	{
		printf("wrong fuse bits\n");
		return 0;
	}
	
	unsigned int r = at_io(0xACA40000 + (fuse_bits & 0xFF));
	int ok = ((r >> 16) & 0xFF) == 0xAC;
	if ( verbose )
	{
		const char *status = ok ? "[ ok ]" : "[ fail ]";
		printf("write device's extended fuse bits %s\n", status);
	}
	
	return ok;
}

/**
* Подать команду "Read Device Code"
*/
unsigned int at_chip_info()
{
	unsigned int sig = 0;
	sig = sig * 256 + (at_io(0x30000000) & 0xFF);
	sig = sig * 256 + (at_io(0x30000100) & 0xFF);
	sig = sig * 256 + (at_io(0x30000200) & 0xFF);
	if ( verbose )
	{
		printf("at_chip_info(): %08X\n", sig);
	}
	return sig;
}

/**
* Действие - вывести информацию об устройстве
*/
int at_act_info()
{
	unsigned int info = at_chip_info();
	printf("chip signature: 0x%02X, 0x%02X, 0x%02X\n", (info >> 16) & 0xFF, (info >> 8) & 0xFF, info & 0xFF);
	return 1;
}

int adc(char pin, double aref, double *result)
{
	uint16_t value = 0;
	int r = cmd_adc(pin & 0x07, &value);
	if ( ! r )
	{
		fprintf(stderr, "ADC fault\n");
		return 0;
	}
	*result = value * (aref / 1024.0);
	return 1;
}

int at_act_adc()
{
	double u;
	int i;
	for(i = 0; i < 10; i++)
	{
	if ( adc(0, 5.05, &u) )
	{
		printf("ADC result: U=%.2f\n", u);
	}
	}
	return 1;
}

int run()
{
	switch ( action )
	{
	case AT_ACT_INFO: return at_act_info();
	case AT_ACT_CHECK: return at_act_check();
	case AT_ACT_WRITE: return at_act_write();
	case AT_ACT_ERASE: return at_act_erase();
	case AT_ACT_READ_FUSE: return at_act_read_fuse();
	case AT_ACT_WRITE_FUSE_LO: return at_act_write_fuse_lo();
	case AT_ACT_WRITE_FUSE_HI: return at_act_write_fuse_hi();
	case AT_ACT_WRITE_FUSE_EX: return at_act_write_fuse_ex();
	case AT_ACT_ADC: return at_act_adc();
	}
	printf("Victory!\n");
	return 0;
}

int help()
{
	printf("pigro :action: :filename: :verbose|quiet:\n");
	printf("  action:\n");
	printf("    info  - read chip info\n");
	printf("    check - read file and compare with device\n");
	printf("    write - read file and write to device\n");
	printf("    erase - just erase chip\n");
	printf("    rfuse - read fuses\n");
	printf("    wfuse_lo - write low fuse bits\n");
	printf("    wfuse_hi - write high fuse bits\n");
	printf("    wfuse_ex - write extended fuse bits\n");
	printf("    adc - run ADC mesure\n");
	return 0;
}


int main(int argc, char *argv[])
{
	int status = 0;
	
	if ( argc <= 1 ) return help();
	
	if ( strcmp(argv[1], "info") == 0 ) action = AT_ACT_INFO;
	else if ( strcmp(argv[1], "check") == 0 ) action = AT_ACT_CHECK;
	else if ( strcmp(argv[1], "write") == 0 ) action = AT_ACT_WRITE;
	else if ( strcmp(argv[1], "erase") == 0 ) action = AT_ACT_ERASE;
	else if ( strcmp(argv[1], "rfuse") == 0 ) action = AT_ACT_READ_FUSE;
	else if ( strcmp(argv[1], "wfuse_lo") == 0 ) action = AT_ACT_WRITE_FUSE_LO;
	else if ( strcmp(argv[1], "wfuse_hi") == 0 ) action = AT_ACT_WRITE_FUSE_HI;
	else if ( strcmp(argv[1], "wfuse_ex") == 0 ) action = AT_ACT_WRITE_FUSE_EX;
	else if ( strcmp(argv[1], "adc") == 0 ) action = AT_ACT_ADC;
	else return help();
	
	fuse_bits = 0x100;
	if ( argc > 2 )
	{
		fuse_bits = at_hex_to_int(argv[2]);
	}
	
	fname = argc > 2 ? argv[2] : "firmware.hex";
	verbose = argc > 3 && strcmp(argv[3], "verbose") == 0;
	if ( verbose )
	{
		printf("firmware file: %s\n", fname);
	}
	
	if ( ! serial_init() )
	{
		printf("serial init fault\n");
		return 1;
	}
	
	if ( action == AT_ACT_ADC )
	{
		status = run() ? 0 : 1;
		
	}
	else if ( at_program_enable() )
	{
		status = run() ? 0 : 1;
	}
	cmd_isp_reset(1);
	
	if ( verbose )
	{
		printf("main() = %d\n", status);
	}
	return status;
}
