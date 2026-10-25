#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_msg.hpp"

/*
===============
MSG_ReadBits

The original reads whatever is in the buffer after the end of the message. The bounds are the ones of ioq3 and OpenJK
(CVE-2017-11721): once the end is passed, a read gives 0 and readcount stays above cursize, which the callers test
The original is still called for the read itself, so one read can go a few bytes past the end before the last test
(Huffman symbols have no fixed length): that is inside the receive buffer, and the value is dropped
===============
*/
int (*Original_MSG_ReadBits)(msg_t*, int);
int Proxy_MSG_ReadBits(msg_t* msg, int bits)
{
	if (msg->readcount > msg->cursize)
	{
		return 0;
	}

	const int bitCount = (bits < 0) ? -bits : bits;

	if (msg->oob)
	{
		if (msg->readcount + (bitCount >> 3) > msg->cursize)
		{
			msg->readcount = msg->cursize + 1;
			return 0;
		}
	}
	else if (msg->bit + (bitCount & 7) > (msg->cursize << 3))
	{
		msg->readcount = msg->cursize + 1;
		return 0;
	}

	const int value = Original_MSG_ReadBits(msg, bits);

	if (!msg->oob && msg->bit > (msg->cursize << 3))
	{
		msg->readcount = msg->cursize + 1;
		return 0;
	}

	return value;
}
