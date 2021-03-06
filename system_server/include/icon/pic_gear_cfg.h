#ifndef _PIC_GEAR_H_
#define _PIC_GEAR_H_

/*
 * 带RAW项
 * - 8K|3D|OF|RAW
 * - 8K|3D|RAW
 * - 8K|RAW
 * - AEB3,5,7,9|RAW
 * - BURST|RAW
 * - CUSTOMER
 */
const u8 pic8K3DOFRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0xEC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x37,
	0x37,0x37,0x37,0x38,0x3F,0x30,0x37,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};

const u8 pic8K3DOF_RAW_Nor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF0,0x10,0x10,0x10,0x10,0x20,0xC0,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x08,
	0x08,0x08,0x08,0x07,0x00,0x0F,0x08,0x08,0x08,0x08,0x04,0x03,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 pic8K3DRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};



const u8 pic8K3DRAWNor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 pic8KRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 pic8KRAWNor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 picAEB3_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB3_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB5_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0xFC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB5_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xF0,0x90,0x90,0x90,0x00,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB7_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0xEC,0xEC,0x6C,0x8C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,0x31,0x3E,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB7_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x10,0x10,0x90,0x70,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,0x0E,0x01,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB9_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x1C,0xEC,0xEC,0xEC,0x1C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3B,0x36,0x36,0x36,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB9_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x04,0x09,0x09,0x09,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picBurstRAWLight_78x16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0x3C,0xFC,0xFC,0xFC,0x3C,0xFC,
	0x3C,0xBC,0xBC,0xFC,0x7C,0xBC,0xBC,0xBC,0x7C,0xFC,0x0C,0xBC,0xBC,0xFC,0x04,0xFC,
	0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x8C,
	0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x30,0x37,0x37,0x37,0x37,0x38,0x3F,0x38,0x37,0x37,0x37,0x30,0x3F,0x30,0x3F,
	0x3F,0x3F,0x3B,0x36,0x36,0x35,0x3B,0x3F,0x38,0x37,0x37,0x3F,0x20,0x3F,0x30,0x3E,
	0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,0x33,
	0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picBurstRAWNor_78x16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,
	0xC0,0x40,0x40,0x00,0x80,0x40,0x40,0x40,0x80,0x00,0xF0,0x40,0x40,0x00,0xF8,0x00,
	0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0x70,
	0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x08,0x08,0x08,0x08,0x07,0x00,0x07,0x08,0x08,0x08,0x0F,0x00,0x0F,0x00,
	0x00,0x00,0x04,0x09,0x09,0x0A,0x04,0x00,0x07,0x08,0x08,0x00,0x1F,0x00,0x0F,0x01,
	0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,0x0C,
	0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



/*
 * 不带RAW项
 * - 8K|3D|OF
 * - 8K|3D
 * - 8K
 * - AEB3,5,7,9
 * - BURST
 * - CUSTOMER
 */


const u8 pic8K3DOFLight_78X16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0xEC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x37,
	0x37,0x37,0x37,0x38,0x3F,0x30,0x37,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 pic8K3DOFNor_78X16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF0,0x10,0x10,0x10,0x10,0x20,0xC0,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x08,
	0x08,0x08,0x08,0x07,0x00,0x0F,0x08,0x08,0x08,0x08,0x04,0x03,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 pic8K3DLight_78x16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,	
};

const u8 pic8K3DNor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 pic8KLight_78x16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,	
};

const u8 pic8KNor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB3Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB3Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x90,0x90,0x90,0x60,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 picAEB5Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB5Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xF0,0x90,0x90,0x90,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB7Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0xEC,0xEC,0x6C,0x8C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,0x31,0x3E,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB7Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x10,0x10,0x90,0x70,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,0x0E,0x01,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



const u8 picAEB9Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x1C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3B,0x36,0x36,0x36,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB9Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x04,0x09,0x09,0x09,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picBurstLight_78x16[] = {
	0x00, 0x00, 0xfc, 0x0c, 0x6c, 0x6c, 0x6c, 0x6c, 0x9c, 0xfc, 0x3c, 0xfc, 0xfc, 0xfc, 0x3c, 0xfc,
	0x3c, 0xbc, 0xbc, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0x7c, 0xfc, 0x0c, 0xbc, 0xbc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00,
	0x3f, 0x30, 0x37, 0x37, 0x37, 0x37, 0x38, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x30, 0x3f, 0x30, 0x3f,
	0x3f, 0x3f, 0x3b, 0x36, 0x36, 0x35, 0x3b, 0x3f, 0x38, 0x37, 0x37, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,	
};

const u8 picBurstNor_78x16[] = {
	0x00, 0x00, 0x00, 0xf0, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc0, 0x00,
	0xc0, 0x40, 0x40, 0x00, 0x80, 0x40, 0x40, 0x40, 0x80, 0x00, 0xf0, 0x40, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0f, 0x08, 0x08, 0x08, 0x08, 0x07, 0x00, 0x07, 0x08, 0x08, 0x08, 0x0f, 0x00, 0x0f, 0x00,
	0x00, 0x00, 0x04, 0x09, 0x09, 0x0a, 0x04, 0x00, 0x07, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


const u8 picVidCustmLight_78x16[ ] =  {
	0x00, 0x00, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0xfc, 0x3c, 0xfc, 0xfc, 0xfc, 0x3c, 0xfc, 0x7c, 0xbc,
	0xbc, 0xbc, 0x7c, 0xfc, 0x0c, 0xbc, 0xbc, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0x7c, 0xfc, 0x3c, 0xbc,
	0xbc, 0x7c, 0xbc, 0xbc, 0x7c, 0xfc, 0x2c, 0xfc, 0xbc, 0xbc, 0xbc, 0xbc, 0x3c, 0xfc, 0x7c, 0xbc,
	0xbc, 0xbc, 0x7c, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00,
	0x3f, 0x38, 0x37, 0x37, 0x37, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x30, 0x3f, 0x3b, 0x36, 0x36, 0x35,
	0x3b, 0x3f, 0x38, 0x37, 0x37, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x38, 0x3f, 0x30, 0x3f, 0x3f, 0x30,
	0x3f, 0x3f, 0x30, 0x3f, 0x30, 0x3f, 0x37, 0x33, 0x35, 0x36, 0x37, 0x3f, 0x38, 0x36, 0x36, 0x36,
	0x36, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
};

const u8  picVidCustmNor_78x16[ ] =  {
	0x00, 0x00, 0x00, 0x80, 0x40, 0x40, 0x40, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x80, 0x40,
	0x40, 0x40, 0x80, 0x00, 0xf0, 0x40, 0x40, 0x00, 0x80, 0x40, 0x40, 0x40, 0x80, 0x00, 0xc0, 0x40,
	0x40, 0x80, 0x40, 0x40, 0x80, 0x00, 0xd0, 0x00, 0x40, 0x40, 0x40, 0x40, 0xc0, 0x00, 0x80, 0x40,
	0x40, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x07, 0x08, 0x08, 0x08, 0x00, 0x07, 0x08, 0x08, 0x08, 0x0f, 0x00, 0x04, 0x09, 0x09, 0x0a,
	0x04, 0x00, 0x07, 0x08, 0x08, 0x00, 0x07, 0x08, 0x08, 0x08, 0x07, 0x00, 0x0f, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x08, 0x0c, 0x0a, 0x09, 0x08, 0x00, 0x07, 0x09, 0x09, 0x09,
	0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


PicVideoCfg pic11K_3D_OF = {
	pItemName:			TAKE_PIC_MODE_11K_3D_OF,		// pItemName
	iItemMaxVal: 		1,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	5,							// 5倍
	stPos:				{0},						// stPos
	pStAction: 			nullptr,					/* 默认值,如果由配置文件可以在初始化时使用配置文件的数据替换 */
	jsonCmd:			nullptr,
	pJsonCmd:			nullptr,
	stLightIcon:		{	/* 选中时的图标列表 */
		pic8K3DOFLight_78X16,
		pic8K3DOFRAWLight_78X16,
	},
	stNorIcon:			{	/* 未选中时的图标列表 */
		pic8K3DOFNor_78X16,
		pic8K3DOF_RAW_Nor_78x16,
	},
    pNote:              "11K|3D|OF",
    bDispType:          false,
};



PicVideoCfg pic11K_OF = {
	pItemName:			TAKE_PIC_MODE_11K_OF,		// pItemName
	iItemMaxVal:		1,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	5,							// 5倍	
	stPos:				{0},						// stPos
	pStAction:			nullptr,
	jsonCmd:			nullptr,
	pJsonCmd:			nullptr,

	stLightIcon:		{	/* 选中时的图标列表 */
		pic8K3DLight_78x16,
		pic8K3DRAWLight_78X16,
	},
	stNorIcon:			{	/* 未选中时的图标列表 */
		pic8K3DNor_78x16,
		pic8K3DRAWNor_78X16
	},
    pNote:              "11K|OF",
    bDispType:          false,
};



PicVideoCfg pic11K = {
	pItemName:			TAKE_PIC_MODE_11K,			// pItemName
	iItemMaxVal:		1,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	5,							// 5倍	
	stPos:				{0},						// stPos

	pStAction:			nullptr,

	jsonCmd:			nullptr,	
	pJsonCmd:			nullptr,

	stLightIcon:		{			/* 选中时的图标列表 */
		pic8KLight_78x16,
		pic8KRAWLight_78X16,
	},
	stNorIcon:			{			/* 未选中时的图标列表 */
		pic8KNor_78x16,
		pic8KRAWNor_78X16
	},
    pNote:              "11K",
    bDispType:          false,
};


PicVideoCfg picAEB = {
	pItemName:			TAKE_PIC_MODE_AEB,			// pItemName
	iItemMaxVal:		7,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	10,							// 10倍
	stPos:				{0},						// stPos
	pStAction:			nullptr,

	jsonCmd:			nullptr,
	pJsonCmd:			nullptr,
		
	stLightIcon:		{	/* 选中时的图标列表 */
		picAEB3Light_78X16,
		picAEB5Light_78X16,
		picAEB7Light_78X16,
		picAEB9Light_78X16,
		picAEB3_RAW_Light_78X16,
		picAEB5_RAW_Light_78X16,
		picAEB7_RAW_Light_78X16,
		picAEB9_RAW_Light_78X16,
	},

	stNorIcon:			{	/* 未选中时的图标列表 */
		picAEB3Nor_78X16,
		picAEB5Nor_78X16,
		picAEB7Nor_78X16,
		picAEB9Nor_78X16,
		picAEB3_RAW_Nor_78X16,
		picAEB5_RAW_Nor_78X16,
		picAEB7_RAW_Nor_78X16,
		picAEB9_RAW_Nor_78X16,
	},
    pNote:              "AEB",
    bDispType:          false,
};


PicVideoCfg picBurst = {
	pItemName:			TAKE_PIC_MODE_BURST,		// pItemName
	iItemMaxVal:		1,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	10,							// 10倍	
	stPos:				{0},						// stPos
	pStAction:			nullptr,
	jsonCmd:			nullptr,
	pJsonCmd:			nullptr,

	stLightIcon:		{	/* 选中时的图标列表 */
		picBurstLight_78x16,
		picBurstRAWLight_78x16,
	},
	stNorIcon:			{	/* 未选中时的图标列表 */
		picBurstNor_78x16,
		picBurstRAWNor_78x16,
	},
    pNote:              "Burst",
    bDispType:          false,
};


PicVideoCfg picCustomer = {
	pItemName:			TAKE_PIC_MODE_CUSTOMER,		// pItemName
	iItemMaxVal:		0,							// iItemMaxVal
	iCurVal:			0,							// iCurVal
	iRawStorageRatio:	5,
	stPos:				{0},						// stPos
	pStAction:			nullptr,
	jsonCmd:			nullptr,
	pJsonCmd:			nullptr,

	stLightIcon:		{	/* 选中时的图标列表 */
		picVidCustmLight_78x16,
	},
	{	/* 未选中时的图标列表 */
		picVidCustmNor_78x16,
	},
    pNote:              "customize",
    bDispType:          false,

};



/*
 * 系统含有SD卡/USB硬件及TF卡时支持的拍照模式（目前只支持该种模式）
 */
PicVideoCfg* gPicAllModeCfgList[] = {
	&pic11K_3D_OF,
	&pic11K_OF,
	&pic11K,
	&picAEB,
	&picBurst,
	&picCustomer,
};



#endif /* _PIC_GEAR_H_ */