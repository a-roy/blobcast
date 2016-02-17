#pragma once

#pragma pack(push, 1)
struct HostData
{
	char HostName[16];
	unsigned char Port[2];
};
#pragma pack(pop)
