//============================================================================
// Name        : NandFlash.c
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Simple function in C, Ansi-style
//============================================================================

#define GpioAdd 0x40013000      //写寄存器地址
#define OE 0x40013010           //方向寄存器地址
#define EXT 0x40013020          //读寄存器地址
#define READADD 10              //读地址偏移量
typedef unsigned short U16;
typedef unsigned char U8;
typedef volatile U16* RP16;
void delay(unsigned int);
int Page_Program(U8,U8,U8,U8,U8*);
int Block_Erase(U8,U8);
U8 Page_Read(U8,U8,U8,U8,U8*);
int main(void)
{
     U8 col_add1,col_add2,row_add1,row_add2;
     int i,j;
     i=0;
     U8 wdata[2048];
     U8 rdata;
	 //选择第512块，32页，0字节处的地址0x4010000
     col_add1 = 0x00;
     col_add2 = 0x00;
     row_add1 = 0x10;
     row_add2 = 0x40;
	 //产生2KB数据，0~255循环
     j=0;
     for(i=0;i<2048;i++)
     {
		wdata[i] = j;
		j++;
		if (j>255){
			j=0;
		}
     }
	 Block_Erase(row_add1,row_add2);
	 Page_Program(col_add1,col_add2,row_add1,row_add2,wdata);
	 Page_Read(col_add1,col_add2,row_add1,row_add2,&rdata);
	 return 0;
}
//块擦除,只要写行地址
int Block_Erase(U8 A12_A19,U8 A20_A27)
{

	*(RP16)(OE) = 0x3ffb;		//给oe寄存器（40013010）置1：使GPIO工作在输出模式

	//设置控制信号：写命令
	*(RP16)(GpioAdd) = 0x0015; //给GPIO的dr寄存器（40013000）写入数据
	*(RP16)(GpioAdd) = 0x1815;			//60h送给IO，设置
	delay(10);						//命令传输最少需要17ns，这边全部扩为10us
	*(RP16)(GpioAdd) = 0x181d;		//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	delay(10);


	*(RP16)(GpioAdd) = 0x1816;		//设置控制信号：写地址

	//写入块地址，因为擦除是以整块为单位的

	//写行地址1
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x1e;
	delay(10);

	//写行地址2
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x1e;
	delay(10);

	*(RP16)(GpioAdd) = 0x0015;				//拉低WE,拉高CLE
	*(RP16)(GpioAdd) = 0x3415;				//发送命令d0h,开始擦除
	delay(10);
	*(RP16)(GpioAdd) = 0x341c;				//拉高WE,拉低CLE
	delay(10);

	while(((*(RP16)(EXT)) & 0x0004) != 0x0004);	//通过R_B信号线判断是否擦除完成

	//写入读状态命令
	*(RP16)(GpioAdd) = 0x0415;
	*(RP16)(GpioAdd) = 0x1c15;				//70h送给IO
	delay(10);

	*(RP16)(GpioAdd) = 0x1c1d;				//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	*(RP16)(GpioAdd) = 0x1c1c;				//CLE拉低
	delay(5);

	*(RP16)(OE) = 0x003b;			//此时将模拟IO口部分的GPIO的oe置0，使其工作在输入模式

	*(RP16)(GpioAdd) = 0x1c0c;				//RE拉低,在RE的下降沿将Read Status寄存器的内容输出在IO管脚上
	delay(10);								//等待读到状态

	if(((*(RP16)(EXT))& 0x0040) == 0x0000)  //检测IO0是否为0。若为0，则擦除成功;
	{
		*(RP16)(GpioAdd) = 0x001c;				//将RE拉高
		return 0;
	}
	else
		return -1;

}

//页编程
int Page_Program(U8 A0_A7,U8 A8_A11,U8 A12_A19,U8 A20_A27,U8 *wdata)
{
	int i;
	*(RP16)(OE) = 0x3ffb;			//R_B置0为输入模式，其余均为输出模式

	//设置控制信号：写命令
	*(RP16)(GpioAdd) = 0x0015;
	//给WE信号设置脉冲信号
	*(RP16)(GpioAdd) = 0x2015;				//80h送给IO,设置
	delay(10);								//命令传输最少需要17ns，这边全部扩为10us
	*(RP16)(GpioAdd) = 0x201d;				//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	delay(10);

	//写入页内地址即2048个字节中的第几个字节
	//写列地址1
	*(RP16)(GpioAdd) = 0x2016;				//设置控制信号：写地址 WE拉低
	*(RP16)(GpioAdd) = (A0_A7<<6)|0x16;		//高8位输入地址
	delay(10);								//低电平持续时间
	*(RP16)(GpioAdd) = (A0_A7<<6)|0x1e;		//WE拉高，在WE的上升沿，地址被锁存到nf的地址锁存器
	delay(10);								//高电平持续时间

	//写列地址2
	*(RP16)(GpioAdd) = (A0_A7<<6)|0x16;		//WE拉低
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x16;	//写入列地址2
	delay(10);
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x1e;	//WE拉高上升沿锁存地址
	delay(10);

	//写行地址1
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x16;
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x1e;
	delay(10);

	//写行地址2
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x16;
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x1e;	//WE拉高
	delay(10);

	//数据通过I/O写入NF的数据寄存器
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x14;	//WE、ALE、CLE拉低进行数据传输
											//数据通过I/O写入NF的数据寄存器
	for(i=0;i<2048;i++)
	{
		*(RP16)(GpioAdd) = (wdata[i]<<6)|0x14;
		delay(10);
		*(RP16)(GpioAdd) = (wdata[i]<<6)|0x1c;		//WE拉高，数据在WE的上升沿被锁存到nf的数据锁存器里
		delay(10);
	}

	//写入命令10h
	*(RP16)(GpioAdd) = wdata[i]<<6|0x15;		   //WE拉低，CLE拉高
	*(RP16)(GpioAdd) = 0x0415;				//10h送给IO,数据从数据寄存器写存贮体
	delay(10);
	*(RP16)(GpioAdd) = 0x041d;				//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	delay(10);                              //twb延迟时间
	while(((*(RP16)(EXT)) & (0x0004)) != 0x0004);	//通过R_B信号线判断是否写入完成

	//写入读状态命令
	*(RP16)(GpioAdd) = 0x1c15;				//WE拉低，CLE拉高,70h送给IO
	delay(10);

	*(RP16)(GpioAdd) = 0x1c1d;				//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	*(RP16)(GpioAdd) = 0x1c1c;				//CLE拉低
	delay(5);								//tCLR+tREA的等待时间

	*(RP16)(OE) = 0x003b;					//此时将模拟IO口部分的GPIO的oe置0，使其工作在输入模式

	*(RP16)(GpioAdd) = 0x1c0c;				//RE拉低,在RE的下降沿将Read Status寄存器的内容输出在IO管脚上
	delay(10);								//等待读到状态

	if((*(RP16)(EXT) & (0x0040))== 0x0000)
	{
		*(RP16)(GpioAdd) = 0x001c;          //将RE拉高
		return 0;							//检测IO0是否为0。若为0，则页编程成功;
	}
	else
		return -1;
}

//整页读
U8 Page_Read(U8 A0_A7,U8 A8_A11,U8 A12_A19,U8 A20_A27, U8 * rdata)
{
	*(RP16)(OE) = 0x3ffb;			//oe置1：输出模式 oe置0：输入模式(R_B)

	//设置控制信号：写命令
	*(RP16)(GpioAdd) = 0x0015;              //设置控制信号
	*(RP16)(GpioAdd) = 0x0015;				//00h送给IO,设置
	delay(10);								//命令传输最少需要17ns，这边全部扩为10us
	*(RP16)(GpioAdd) = 0x001d;				//WE拉高，在WE的上升沿，命令被锁存到nf的命令锁存器
	delay(10);

	//写列地址1
	*(RP16)(GpioAdd) = 0x0016;				//设置控制信号：写地址 WE拉低
	*(RP16)(GpioAdd) = ((A0_A7+READADD)<<6)|0x16;		//高8位输入地址+偏移量地址
	delay(10);								//低电平持续时间
	*(RP16)(GpioAdd) = ((A0_A7+READADD)<<6)|0x1e;		//WE拉高，在WE的上升沿，地址被锁存到nf的地址锁存器
	delay(10);								//高电平持续时间

	//写列地址2
	*(RP16)(GpioAdd) = ((A0_A7+READADD)<<6)|0x16;
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x1e;
	delay(10);

	//写行地址1
	*(RP16)(GpioAdd) = (A8_A11<<6)|0x16;
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x1e;
	delay(10);

	//写行地址2
	*(RP16)(GpioAdd) = (A12_A19<<6)|0x16;
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x16;
	delay(10);
	*(RP16)(GpioAdd) = (A20_A27<<6)|0x1e;	//WE拉高
	delay(10);


	*(RP16)(GpioAdd) = 0x0015;
	*(RP16)(GpioAdd) = 0x0c15;				//发送命令30h
	delay(10);
	*(RP16)(GpioAdd) = 0x0c1c;				//拉低ALE、CLE，拉高WE
											//数据从存储体传送到数据寄存器中
	while(((*(RP16)(GpioAdd)) & (0x0004)) != 0x0004);	//通过R_B信号线判断是否传送是否完成
	delay(10);

	*(RP16)(OE) = 0x003b;			//IO全部变成输入模式用于读取数据寄存器中的数据
	*(RP16)(GpioAdd) = 0x000c;				//RE拉低
	delay(10);
	*rdata = *(RP16)(EXT)>>6;
	*(RP16)(GpioAdd) = 0x001c;				//RE拉高，读出数据
	delay(10);

	*(RP16)(GpioAdd) = 0x003c;				//CE拉高，取消片选，结束
	return 0;
}

//延时函数：晶振若为50M，则j=1时刚好延迟1us
void delay(unsigned int j)
{
	unsigned int a,b,c;
		for(a=j;a>0;a--)
			for(b=0;b<1;b++)
				for(c=0;c<50;c++);
}
