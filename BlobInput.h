#pragma once

enum BlobInput : unsigned char
{
	NoInput  = 0x00,
	Forward  = 0x01,
	Backward = 0x02,
	Right    = 0x04,
	Left     = 0x08,
	Jump     = 0x10
};
