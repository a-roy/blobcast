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

struct AggregateInput
{
	int FCount = 0;
	int BCount = 0;
	int RCount = 0;
	int LCount = 0;
	int FRCount = 0;
	int FLCount = 0;
	int BRCount = 0;
	int BLCount = 0;
	int JCount = 0;
	int TotalCount = 0;

	AggregateInput operator+(const BlobInput& input)
	{
		AggregateInput i = *this;
		i += input;
		return i;
	}

	AggregateInput& operator+=(const BlobInput& input)
	{
		BlobInput ZDir = (BlobInput)(
			(((input & Forward) == 0) == ((input & Backward) == 0)) ? NoInput :
			input & (Forward | Backward));
		BlobInput XDir = (BlobInput)(
			(((input & Right) == 0) == ((input & Left) == 0)) ? NoInput :
			input & (Left | Right));
		if (ZDir == Forward && XDir == NoInput)
			FCount++;
		else if (ZDir == Backward && XDir == NoInput)
			BCount++;
		else if (XDir == Right && ZDir == NoInput)
			RCount++;
		else if (XDir == Left && ZDir == NoInput)
			LCount++;
		else if (ZDir == Forward && XDir == Right)
			FRCount++;
		else if (ZDir == Forward && XDir == Left)
			FLCount++;
		else if (ZDir == Backward && XDir == Right)
			BRCount++;
		else if (ZDir == Backward && XDir == Left)
			BLCount++;
		if (input & Jump)
			JCount++;
		TotalCount++;

		return *this;
	}
};
