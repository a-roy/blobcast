#pragma once

#include "BlobInput.h"

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

	AggregateInput operator+(const BlobInput& input);
	AggregateInput& operator+=(const BlobInput& input);
};
