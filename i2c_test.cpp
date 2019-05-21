#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <wiringPi.h>

#define Motor0_A  0
#define Motor0_B  1
#define Motor1_A  2
#define Motor1_B  3
#define I2C_ADDR 0x40
#define Freq 60

void motors_setup()
{
	pinMode(Motor0_A, OUTPUT);
	pinMode(Motor0_B, OUTPUT);
	pinMode(Motor1_A, OUTPUT);
	pinMode(Motor1_B, OUTPUT);
}

void write_data_by_2(int &fd, __u8 _addr, __u8 _data)
{
	__u8 _msgs[2];
	_msgs[0] = _addr;
	_msgs[1] = _data;
	struct i2c_msg msgs;
	msgs.addr = I2C_ADDR;
	msgs.flags = 0;
	msgs.len = 2;
	msgs.buf = _msgs;
	struct i2c_rdwr_ioctl_data data;
	data.msgs = &msgs;
	data.nmsgs = 1;
	ioctl(fd, I2C_RDWR, &data);
}

int PCA9685_setup(const char* pathname, int oflag)
{
	int fd = open(pathname, oflag);
	if (fd < 0)
	{
		printf("Open PCA9685 error!");
		return 1;
	}
	ioctl(fd, I2C_TIMEOUT, 10);
	ioctl(fd, I2C_RETRIES, 2);

	//sleep
	write_data_by_2(fd, 0x00, 0x10);

	//set frequency
	float prescale_value = 25000000.0;
	prescale_value /= 4096.0;
	prescale_value /= Freq;
	prescale_value -= 1.0;
	write_data_by_2(fd, 0xfe, (int)(prescale_value + 0.5));

	//set mode
	write_data_by_2(fd, 0x00, 0x00);

	return fd;
}

void write_channel_data(int &fd, __u8 _channel, __u16 _data)
{
	printf("channel: %d    data: %d\n", _channel, _data);
	write_data_by_2(fd, 0x06 + 4 * _channel, 0x00);
	write_data_by_2(fd, 0x06 + 4 * _channel + 1, 0x00);
	write_data_by_2(fd, 0x06 + 4 * _channel + 2, _data & 0xff);
	write_data_by_2(fd, 0x06 + 4 * _channel + 3, _data >> 8);
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("usage: ./i2c_test data1 data2\n");
		return -1;
	}

	if (wiringPiSetup() == -1) { 
		printf("setup wiringPi failed !");
		return 1;
	}

	//set motors
	motors_setup();
	digitalWrite(Motor0_A, 1);
	digitalWrite(Motor0_B, 0);
	digitalWrite(Motor1_A, 0);
	digitalWrite(Motor1_B, 1);

	int fd;
	fd = PCA9685_setup("/dev/i2c-1", O_RDWR);
	write_channel_data(fd, 15, 400);	//top
	write_channel_data(fd, 5, 40 * atoi(argv[1]));	//left
	write_channel_data(fd, 4, 40 * atoi(argv[2]));	//right

	close(fd);

	return 0;
}
