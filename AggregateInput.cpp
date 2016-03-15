#include "AggregateInput.h"

AggregateInput AggregateInput::operator+(const BlobInput& input)
{
	AggregateInput i = *this;
	i += input;
	return i;
}

AggregateInput& AggregateInput::operator+=(const BlobInput& input)
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
